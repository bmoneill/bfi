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

#define MAX_LOOPS 2048
#define TAPE_SIZE 30000
#define INPUT_MAX 1024

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Structure to represent an index in a file (or user input).
 */
typedef struct {
    int idx;
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

/* Flags */
bool debug = false;
bool repl  = false;

/* Global interpreter variables */
char*       prog;
uint8_t     tape[TAPE_SIZE];
int         ip     = 0;
int         tp     = 0;
int         tp_max = 0;
loop_t      loops[MAX_LOOPS];
int         program_len;
int         num_loops;

static void build_loops(void);
static void diagnose(file_index_t*);
static void interpret(file_index_t*);
static int  load_file(const char*);
static void print_err(const char*);
static void print_usage(const char*);
static void run_file(void);
static void run_repl(void);

/**
 * @brief Entry point.
 */
int main(int argc, char* argv[]) {
    char* path = NULL;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; j < strlen(argv[i]); j++) {
                switch (argv[i][j]) {
                case 'd':
                    debug = true;
                    break;
                case 'r':
                    repl = true;
                    break;
                }
            }
        } else {
            if (path == NULL) {
                path = argv[i];
            } else {
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
        }
    }

    if (!repl && path) {
        load_file(path);
        run_file();
    } else if (repl && !path) {
        run_repl();
    } else {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
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
static void build_loops(void) {
    num_loops = 0;
    file_index_t stack[MAX_LOOPS];
    int          stack_top = 0;
    int          line      = 1;
    int          line_idx  = 0;

    for (int i = 0; i < program_len; i++) {
        line_idx++;
        if (prog[i] == '[') {
            if (stack_top >= MAX_LOOPS) {
                fprintf(stderr,
                        "Error (%d,%d): Too many loops (max = %d).",
                        line,
                        line_idx,
                        MAX_LOOPS);
                exit(EXIT_FAILURE);
            }
            stack[stack_top].idx      = i;
            stack[stack_top].line     = line;
            stack[stack_top].line_idx = line_idx;
            stack_top++;
        } else if (prog[i] == ']') {
            if (stack_top <= 0) {
                fprintf(stderr, "Error (%d,%d): Unmatched closing bracket ']'.\n", line, line_idx);
                exit(EXIT_FAILURE);
            }
            file_index_t start = stack[--stack_top];
            if (num_loops >= MAX_LOOPS) {
                fprintf(stderr,
                        "Error (%d,%d): Too many loops (max = %d).",
                        line,
                        line_idx,
                        MAX_LOOPS);
                exit(EXIT_FAILURE);
            }
            loops[num_loops].start        = start;
            loops[num_loops].end.idx      = i;
            loops[num_loops].end.line     = line;
            loops[num_loops].end.line_idx = line_idx;
            num_loops++;
        } else if (prog[i] == '\n') {
            line++;
            line_idx = 0;
        }
    }
    if (stack_top != 0) {
        fprintf(stderr, "Error (%d,%d): Unmatched opening bracket '['.\n", line, line_idx);
        exit(EXIT_FAILURE);
    }
}

static void diagnose(file_index_t* idx) {
    fprintf(stderr,
            "Line: %d,%d\nTape pointer: %d\nInstruction pointer: %d\n",
            idx->line,
            idx->line_idx,
            tp,
            ip);

    /* print memory map */
    for (int i = 0; i < tp_max; i++) {
        fprintf(stderr, "%d: %d\n", i, tape[i]);
    }
}

/**
 *  @brief Interprets a single instruction in the brainfuck program.
 *
 *  This function reads the current instruction pointed to by the instruction pointer (ip)
 *  and performs the corresponding operation on the tape.
 */
static void interpret(file_index_t* index) {
    char c;
    bool receiving = true;

    index->line_idx++;
    switch (prog[ip]) {
    case '+':
        tape[tp]++;
        break;
    case '-':
        tape[tp]--;
        break;
    case '>':
        tp++;
        if (tp > TAPE_SIZE) {
            fprintf(stderr,
                    "Warning (%d,%d): Tape pointer overflow. Tape pointer set to zero.\n",
                    index->line,
                    index->line_idx);
            tp = 0;
        } else if (tp > tp_max) {
            tp_max = tp;
        }
        break;
    case '<':
        tp--;
        if (tp < 0) {
            fprintf(stderr,
                    "Warning (%d,%d): Tape pointer underflow. Tape pointer set to zero.\n",
                    index->line,
                    index->line_idx);
            tp = 0;
        }
        break;
    case ',':
        if (receiving) {
            c = fgetc(stdin);
            if (c == EOF) {
                c         = 0; // EOF will overflow on uint8_t
                receiving = false;
            }
        }
        tape[tp] = c;
        break;
    case '.':
        putchar(tape[tp]);
        break;
    case '[':
        if (!tape[tp]) {
            for (int i = 0; i < num_loops; i++) {
                if (loops[i].start.idx == ip) {
                    ip              = loops[i].end.idx;
                    index->line     = loops[i].end.line;
                    index->line_idx = loops[i].end.line_idx;
                }
            }
        }
        break;
    case ']':
        if (tape[tp]) {
            for (int i = 0; i < num_loops; i++) {
                if (loops[i].end.idx == ip) {
                    ip              = loops[i].start.idx;
                    index->line     = loops[i].start.line;
                    index->line_idx = loops[i].start.line_idx;
                }
            }
        }
        break;
    case '#':
        if (debug) {
            diagnose(index);
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
static int load_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        program_len = ftell(f);
        fseek(f, 0, SEEK_SET);

        prog = (char*) malloc(program_len);
        if (prog) {
            fread(prog, 1, program_len, f);
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
 * @brief Prints the usage message for the program.
 *
 * @param argv0 The name of the program as it was invoked.
 */
static void print_usage(const char* argv0) { fprintf(stderr, "usage: %s [-dr] [file]\n", argv0); }

/**
 * @brief Runs the brainfuck program loaded from a file.
 *
 * This function builds the loop structure for the brainfuck program,
 * then iterates through the program instructions, interpreting each one
 * until the end of the program is reached.
 */
static void run_file(void) {
    file_index_t idx;
    idx.line     = 1;
    idx.line_idx = 0;

    build_loops();
    for (ip = 0; ip < program_len; ip++) {
        interpret(&idx);
    }
    free(prog);
}

/**
 * @brief Runs the brainfuck interpreter in REPL (Read-Eval-Print Loop) mode.
 *
 * This function continuously reads input from the user, appends it to the program,
 * and interprets the brainfuck instructions until the user terminates the program.
 * This allows for interactive execution of brainfuck code.
 */
static void run_repl(void) {
    char input[INPUT_MAX];
    prog = (char*) malloc(INPUT_MAX);

    if (!prog) {
        fprintf(stderr, "Error: Cannot allocate memory for program storage.\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        size_t program_len_old = 0;
        printf("> ");
        fgets(input, INPUT_MAX, stdin);
        program_len_old = program_len;
        program_len += strlen(input);
        snprintf(prog + program_len_old, INPUT_MAX - program_len_old, "%s", input);
        build_loops();

        file_index_t index;
        index.line     = 1;
        index.line_idx = 0;

        for (; ip < program_len; ip++)
            interpret(&index);
    }

    free(prog);
}
