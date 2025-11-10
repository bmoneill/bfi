/**
 * @file bfi.c
 * @brief brainfuck interpreter
 * @author Ben O'Neill <ben@oneill.sh>
 *
 * This is a simple brainfuck interpreter with support for file
 * execution and REPL mode.
 *
 * @copyright Copyright (c) 2022-2025 Ben O'Neill <ben@oneill.sh>.
 * This work is released under the terms of the MIT License. See
 * LICENSE.
 */

#include "bf.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

/* Compiler stuff */
#define COMPILE_HEAD  "#include <stdio.h>\nint main(void) {unsigned char t[%ld];int p=0;"
#define TMP_FILE_PATH "/tmp/bfi.c"

/* Interpreter stuff */
#define INITIAL_LOOP_SIZE        2048
#define DEFAULT_INPUT_STACK_SIZE 16

#define ERROR(s)                                                                                   \
    fprintf(stderr, "Error: %s\n", s);                                                             \
    exit(EXIT_FAILURE)
#define IN_REPL_MODE(b)                 ((b).flags & BF_FLAG_REPL)
#define IN_DEBUG_MODE(b)                ((b).flags & BF_FLAG_DEBUG)
#define SPECIAL_INSTRUCTIONS_ENABLED(b) (!((b).flags & BF_FLAG_DISABLE_SPECIAL_INSTRUCTIONS))

/**
 * @brief Structure to represent an index in a file (or user input).
 */
typedef struct {
    int idx; /* only used for build_loops() */
    int line;
    int line_idx;
} file_index_t;

/**
 *  @brief Structure to represent a loop in the brainfuck program.
 *   This structure holds the start and end indices of a loop,
 *   allowing the interpreter to efficiently jump between matching
 *   brackets during execution.
 *   @note The `start` field represents the index of the opening bracket '[',
 *         and the `end` field represents the index of the closing bracket ']'.
 */
typedef struct {
    file_index_t start;
    file_index_t end;
} loop_t;

/**
 * @brief Structure to represent a brainfuck interpreter.
 * @param flags Flags for the interpreter.
 * @param prog Pointer to the brainfuck program string.
 * @param prog_len Length of the brainfuck program string.
 * @param prog_size Size of the brainfuck program string.
 * @param tape Pointer to the tape array.
 * @param tape_size Size of the tape array.
 * @param ip Instruction pointer.
 * @param tp Data pointer.
 * @param tp_max Maximum data pointer value.
 * @param loops Pointer to the loop array.
 * @param loops_len Length of the loop array.
 * @param loops_size Size of the loop array.
 */
typedef struct {
    int      flags;
    char*    prog;
    int      prog_len;
    size_t   prog_size;
    uint8_t* tape;
    size_t   tape_size;
    int      ip;
    int      tp;
    int      tp_max;
    loop_t*  loops;
    int      loops_len;
    size_t   loops_size;
} bf_t;

static void build_loops(bf_t*);
static void diagnose(bf_t*, file_index_t*);
static void free_bf(bf_t*);
static void init_bf(bf_t*, bf_parameters_t);
static void init_tokens(void);
static void interpret(bf_t*, file_index_t*);
static int  load_file(bf_t*, const char*);
static void reset(bf_t*);
static void reset_loops(bf_t*);

const char* tokens[']' + 1];

/**
 * @brief Compile Brainfuck code from input_path to output_path.
 *
 * This function takes a Brainfuck source code file specified by input_path,
 * compiles it into a C program, compiles the C program, and writes the resulting code to the file
 * specified by output_path.
 * If input_path is NULL, the function will read from stdin. If output_path is NULL, the function
 * will write to ./a.out(.c).
 *
 * @param input_path Path to the input Brainfuck source code file.
 * @param output_path Path to the output binary or C file.
 * @param params Compilation parameters
 */
void bf_compile(const char* input_path, const char* output_path, bf_parameters_t params) {
    FILE* input;
    FILE* output;
    bool  binary_output = !(params.flags & BF_FLAG_ONLY_GENERATE_C_SOURCE);
    int   c;
    int   depth;

    /**** Set up files ****/
    if (!input_path) {
        input = stdin;
    } else if (!(input = fopen(input_path, "r"))) {
        ERROR("Failed to open input file");
    }

    if (!output_path) {
        output_path = binary_output ? "./a.out" : "./a.out.c";
    }

    if (binary_output && !(output = fopen(TMP_FILE_PATH, "w"))) {
        ERROR("Failed to create temporary file");
    } else if (!(output = fopen(output_path, "w"))) {
        ERROR("Failed to open output file");
    }

    /*** Actual compilation ***/
    init_tokens();
    depth = 0;
    fprintf(output, COMPILE_HEAD, params.tape_size);
    while ((c = fgetc(input)) != EOF) {
        if (tokens[c]) {
            fprintf(output, "%s", tokens[c]);
        }
    }

    if (depth != 0) {
        fclose(output);
        remove(output_path);
        ERROR("Unbalanced brackets");
    }

    fprintf(output, "return 0;}");
    fclose(output);

    if (binary_output) {
        char* cmd = malloc(128);
        sprintf(cmd,
                "%s %s -o %s %s",
                BF_DEFAULT_COMPILER,
                BF_DEFAULT_COMPILE_FLAGS,
                output_path,
                TMP_FILE_PATH);
        system(cmd);
        remove(TMP_FILE_PATH);
    }
}

/**
 * @brief Runs the brainfuck program loaded from a file.
 *
 * This function builds the loop structure for the brainfuck program,
 * then iterates through the program instructions, interpreting each one
 * until the end of the program is reached.
 */
void bf_run_file(const char* path, bf_parameters_t params) {
    bf_t         bf;
    file_index_t idx;

    init_bf(&bf, params);
    load_file(&bf, path);
    idx.line     = 1;
    idx.line_idx = 0;
    build_loops(&bf);

    for (bf.ip = 0; bf.ip < bf.prog_len; bf.ip++) {
        interpret(&bf, &idx);
    }
    free(bf.prog);
}

/**
 * @brief Runs the brainfuck interpreter in REPL (Read-Eval-Print Loop) mode.
 *
 * This function continuously reads input from the user, appends it to the program,
 * and interprets the brainfuck instructions until the user terminates the program.
 * This allows for interactive execution of brainfuck code.
 */
void bf_run_repl(bf_parameters_t params) {
    bf_t         bf;
    file_index_t index;
    char*        input;
    size_t       prog_len_old;

    init_bf(&bf, params);

    bf.prog_size = params.input_max;

    if (!(bf.prog = (char*) malloc(bf.prog_size + 1))
        || !(input = (char*) malloc(bf.prog_size + 1))) {
        fprintf(stderr, "Error: Cannot allocate memory for program storage.\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        printf("> ");
        if (!fgets(input, params.input_max, stdin)) {
            break;
        }

        prog_len_old = bf.prog_len;
        bf.prog_len += strlen(input);
        if ((size_t) bf.prog_len > bf.prog_size) {
            bf.prog_size *= 2;
            if (!(bf.prog = realloc(bf.prog, bf.prog_size))) {
                fprintf(stderr, "Error: Cannot reallocate memory for program storage.\n");
                exit(EXIT_FAILURE);
            }
        }

        snprintf(bf.prog + prog_len_old, bf.prog_size - prog_len_old, "%s", input);
        reset_loops(&bf);
        build_loops(&bf);
        index.line     = 1;
        index.line_idx = 0;
        for (; bf.ip < bf.prog_len; bf.ip++) {
            interpret(&bf, &index);
        }
    }

    free(input);
    free_bf(&bf);
}

/**
 * @brief Builds the loop structure for the brainfuck interpreter.
 *
 * This function scans the brainfuck program and constructs the necessary
 * data structures to efficiently handle loop constructs ('[' and ']').
 * It ensures that matching brackets are correctly paired, allowing for
 * proper execution flow during interpretation.
 *
 * @note This function does not return a value. If an error occurs (such as exceeding MAX_LOOPS),
 *       it prints an error message and terminates the program using exit(EXIT_FAILURE).
 */
static void build_loops(bf_t* bf) {
    file_index_t* stack;
    file_index_t  start;
    int           stack_top;
    int           stack_size;
    int           line;
    int           line_idx;
    int           i;

    stack          = malloc(sizeof(file_index_t) * INITIAL_LOOP_SIZE);
    stack_top      = 0;
    stack_size     = INITIAL_LOOP_SIZE;
    line           = 1;
    line_idx       = 0;
    bf->loops_len  = 0;
    bf->loops_size = INITIAL_LOOP_SIZE;
    bf->loops      = malloc(sizeof(loop_t) * INITIAL_LOOP_SIZE);

    for (i = 0; i < bf->prog_len; i++) {
        line_idx++;
        if (bf->prog[i] == '[') {
            if (stack_top >= stack_size) {
                stack_size *= 2;
                stack = realloc(stack, sizeof(file_index_t) * stack_size);
            }
            stack[stack_top].idx      = i;
            stack[stack_top].line     = line;
            stack[stack_top].line_idx = line_idx;
            stack_top++;
        } else if (bf->prog[i] == ']') {
            if (stack_top <= 0) {
                fprintf(stderr, "Error (%d,%d): Unmatched closing bracket ']'.\n", line, line_idx);
                free(stack);
                exit(EXIT_FAILURE);
            }
            start                                 = stack[--stack_top];
            bf->loops[bf->loops_len].start        = start;
            bf->loops[bf->loops_len].end.idx      = i;
            bf->loops[bf->loops_len].end.line     = line;
            bf->loops[bf->loops_len].end.line_idx = line_idx;
            bf->loops_len++;
        } else if (bf->prog[i] == '\n') {
            line++;
            line_idx = 0;
        }
    }

    if (stack_top != 0) {
        fprintf(stderr, "Error (%d,%d): Unmatched opening bracket '['.\n", line, line_idx);
        free(stack);
        exit(EXIT_FAILURE);
    }

    free(stack);
}

/**
 * @brief Diagnoses the brainfuck program.
 *
 * This function prints the current state of the brainfuck program, including the line number,
 * tape pointer, instruction pointer, and memory map.
 */
static void diagnose(bf_t* bf, file_index_t* idx) {
    int i;

    fprintf(stderr,
            "Line: %d,%d\nTape pointer: %d\nInstruction pointer: %d\n",
            idx->line,
            idx->line_idx,
            bf->tp,
            bf->ip);

    fprintf(stderr, "Memory map:\n");
    for (i = 0; i < bf->tp_max; i++) {
        fprintf(stderr, "%d: %d\n", i, bf->tape[i]);
    }
}

/**
 * @brief Frees the memory allocated for the brainfuck program.
 * @param bf Pointer to the brainfuck program.
 */
static void free_bf(bf_t* bf) {
    if (bf) {
        if (bf->prog) {
            free(bf->prog);
        }
        if (bf->tape) {
            free(bf->tape);
        }
        if (bf->loops) {
            free(bf->loops);
        }
    }
}

/**
 * @brief Initializes a bf_t.
 * @param bf Pointer to the bf_t
 * @param params Parameters for the bf_t.
 */
static void init_bf(bf_t* bf, bf_parameters_t params) {
    memset(bf, 0, sizeof(bf_t));
    bf->flags     = params.flags;
    bf->tape_size = params.tape_size;
    bf->tape      = calloc(params.tape_size, sizeof(uint8_t));
}

/**
 * @brief Initializes compiler tokens
 */
static void init_tokens(void) {
    memset(tokens, 0, (']' + 1) * sizeof(char*));
    tokens['>'] = "p++;";
    tokens['<'] = "p--;";
    tokens['+'] = "t[p]++;";
    tokens['-'] = "t[p]--;";
    tokens['.'] = "putchar(t[p]);";
    tokens[','] = "t[p]=getchar();";
    tokens['['] = "while(t[p]){";
    tokens[']'] = "}";
}
/**
 *  @brief Interprets a single instruction in the brainfuck program.
 *
 *  This function reads the current instruction pointed to by the instruction pointer
 *  and performs the corresponding operation on the tape.
 */
static void interpret(bf_t* bf, file_index_t* index) {
    int  i;
    char c;
    bool receiving;

    receiving = true;
    index->line_idx++;
    switch (bf->prog[bf->ip]) {
    case '+':
        bf->tape[bf->tp]++;
        break;
    case '-':
        bf->tape[bf->tp]--;
        break;
    case '>':
        bf->tp++;
        if ((size_t) bf->tp > bf->tape_size) {
            fprintf(stderr,
                    "Warning (%d,%d): Tape pointer overflow. Tape pointer set to zero.\n",
                    index->line,
                    index->line_idx);
            bf->tp = 0;
        } else if (bf->tp > bf->tp_max) {
            bf->tp_max = bf->tp;
        }
        break;
    case '<':
        bf->tp--;
        if (bf->tp < 0) {
            fprintf(stderr,
                    "Warning (%d,%d): Tape pointer underflow. Tape pointer set to zero.\n",
                    index->line,
                    index->line_idx);
            bf->tp = 0;
        }
        break;
    case ',':
        if (receiving) {
            c = fgetc(stdin);
            if (c == EOF) {
                c         = 0; /* EOF will overflow on uint8_t */
                receiving = false;
            }
        }
        bf->tape[bf->tp] = c;
        break;
    case '.':
        putchar(bf->tape[bf->tp]);
        break;
    case '[':
        if (!bf->tape[bf->tp]) {
            for (i = 0; i < bf->loops_len; i++) {
                if (bf->loops[i].start.idx == bf->ip) {
                    bf->ip          = bf->loops[i].end.idx;
                    index->line     = bf->loops[i].end.line;
                    index->line_idx = bf->loops[i].end.line_idx;
                }
            }
        }
        break;
    case ']':
        if (bf->tape[bf->tp]) {
            for (i = 0; i < bf->loops_len; i++) {
                if (bf->loops[i].end.idx == bf->ip) {
                    bf->ip          = bf->loops[i].start.idx;
                    index->line     = bf->loops[i].start.line;
                    index->line_idx = bf->loops[i].start.line_idx;
                }
            }
        }
        break;
    case '#':
        if (IN_DEBUG_MODE(*bf) && SPECIAL_INSTRUCTIONS_ENABLED(*bf)) {
            diagnose(bf, index);
        }
        break;
    case '@':
        if (IN_REPL_MODE(*bf) && SPECIAL_INSTRUCTIONS_ENABLED(*bf)) {
            reset(bf);
        }
        break;
    case '\n':
        index->line++;
        index->line_idx = 0;
        break;
    }
}

/**
 *  @brief Loads a brainfuck program from a file into memory.
 *
 *   This function opens the specified file, reads its contents into a dynamically allocated buffer,
 *   and stores the program length. If the file cannot be opened or memory allocation fails,
 *   it prints an error message and returns a non-zero value to indicate failure.
 *
 *  @param path The path to the brainfuck program file.
 *
 *  @return Returns 0 on success, or 1 if an error occurs (e.g., file not found, memory allocation failure).
 *
 *  @note The program is expected to be in plain text format, with brainfuck instructions.
 *        The function allocates memory for the program and reads the entire file into this buffer.
 *        The caller is responsible for freeing the allocated memory.
 */
static int load_file(bf_t* bf, const char* path) {
    FILE* f;

    if ((f = fopen(path, "r"))) {
        fseek(f, 0, SEEK_END);
        bf->prog_len = ftell(f);
        fseek(f, 0, SEEK_SET);

        if ((bf->prog = (char*) malloc(bf->prog_len))) {
            fread(bf->prog, 1, bf->prog_len, f);
        } else {
            fprintf(stderr, "Error: Cannot allocate memory for program storage.\n");
            fclose(f);
            return 1;
        }
        fclose(f);
    } else {
        fprintf(stderr, "Error: Cannot open file %s for reading.\n", path);
        return 1;
    }

    return 0;
}

/**
 * @brief Resets the brainfuck program state.
 *
 * This function resets the program state by clearing the program buffer,
 * tape, and loop structure, and resetting the instruction pointer and tape pointer.
 */
static void reset(bf_t* bf) {
    reset_loops(bf);
    memset(bf->prog, 0, bf->prog_len * sizeof(char));
    memset(bf->tape, 0, bf->tape_size * sizeof(uint8_t));
    bf->prog_len = 0;
    bf->ip       = 0;
    bf->tp       = 0;
    bf->tp_max   = 0;
}

/**
 * @brief Resets the loop structure.
 *
 * This function resets the loop structure by clearing the loop buffer.
 */
static void reset_loops(bf_t* bf) {
    memset(bf->loops, 0, bf->loops_len);
    bf->loops_len = 0;
}
