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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#define LOOPS            2048
#define TAPE_SIZE        30000
#define INPUT_MAX        1024
#define INPUT_STACK_SIZE 16
#define FLAG_DEBUG       1
#define FLAG_REPL        2

/**
 * @brief Structure to represent an index in a file (or user input).
 */
typedef struct {
    int idx; // only used for build_loops()
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
    int      tape_size;
    int      ip;
    int      tp;
    int      tp_max;
    loop_t*  loops;
    size_t   loops_len;
    int      loops_size;
} bf_t;

static void build_loops(bf_t*);
static void diagnose(bf_t*, file_index_t*);
static void free_brainfuck(bf_t*);
static void interpret(bf_t*, file_index_t*);
static int  load_file(bf_t*, const char*);
static void reset(bf_t*);

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
void build_loops(bf_t* bf) {
    bf->loops_len            = 0;
    bf->loops_size           = INITIAL_LOOP_SIZE;
    bf->loops                = malloc(sizeof(loop_t) * INITIAL_LOOP_SIZE);
    file_index_t* stack      = malloc(sizeof(file_index_t) * INITIAL_LOOP_SIZE);
    int           stack_top  = 0;
    int           stack_size = INITIAL_LOOP_SIZE;
    int           line       = 1;
    int           line_idx   = 0;

    for (int i = 0; i < bf->prog_len; i++) {
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
                exit(EXIT_FAILURE);
            }
            file_index_t start                    = stack[--stack_top];
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
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Diagnoses the brainfuck program.
 *
 * This function prints the current state of the brainfuck program, including the line number,
 * tape pointer, instruction pointer, and memory map.
 */
void diagnose(bf_t* bf, file_index_t* idx) {
    fprintf(stderr,
            "Line: %d,%d\nTape pointer: %d\nInstruction pointer: %d\n",
            idx->line,
            idx->line_idx,
            bf->tp,
            bf->ip);

    printf("Memory map:\n");
    for (int i = 0; i < bf->tp_max; i++) {
        fprintf(stderr, "%d: %d\n", i, bf->tape[i]);
    }
}

/**
 * @brief Frees the memory allocated for the brainfuck program.
 * @param bf Pointer to the brainfuck program.
 */
void free_brainfuck(bf_t* bf) {
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
        free(bf);
    }
}

/**
 *  @brief Interprets a single instruction in the brainfuck program.
 *
 *  This function reads the current instruction pointed to by the instruction pointer (ip)
 *  and performs the corresponding operation on the tape.
 */
void interpret(bf_t* bf, file_index_t* index) {
    char c;
    bool receiving = true;

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
        if (bf->tp > bf->tape_size) {
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
                c         = 0; // EOF will overflow on uint8_t
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
            for (int i = 0; i < bf->loops_len; i++) {
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
            for (int i = 0; i < bf->loops_len; i++) {
                if (bf->loops[i].end.idx == bf->ip) {
                    bf->ip          = bf->loops[i].start.idx;
                    index->line     = bf->loops[i].start.line;
                    index->line_idx = bf->loops[i].start.line_idx;
                }
            }
        }
        break;
    case '#':
        if (IS_DEBUG_MODE(*bf)) {
            diagnose(bf, index);
        }
        break;
    case '@':
        if (IS_REPL_MODE(*bf)) {
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
int load_file(bf_t* bf, const char* path) {
    FILE* f = fopen(path, "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        bf->prog_len = ftell(f);
        fseek(f, 0, SEEK_SET);

        bf->prog = (char*) malloc(bf->prog_len);
        if (bf->prog) {
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
 * @brief Prints the usage message for the program.
 *
 * @param argv0 The name of the program as it was invoked.
 */
void print_usage(const char* argv0) { fprintf(stderr, "usage: %s [-dr] [file]\n", argv0); }

void reset(bf_t* bf) {
    memset(bf->prog, 0, bf->prog_len * sizeof(char));
    memset(bf->tape, 0, bf->tape_size * sizeof(uint8_t));
    memset(bf->loops, 0, bf->loops_len);
    bf->prog_len  = 0;
    bf->loops_len = 0;
    bf->ip        = 0;
    bf->tp        = 0;
    bf->tp_max    = 0;
}

/**
 * @brief Runs the brainfuck program loaded from a file.
 *
 * This function builds the loop structure for the brainfuck program,
 * then iterates through the program instructions, interpreting each one
 * until the end of the program is reached.
 */
void run_file(bf_t* bf) {
    file_index_t idx;
    idx.line     = 1;
    idx.line_idx = 0;

    build_loops(bf);
    for (bf->ip = 0; bf->ip < bf->prog_len; bf->ip++) {
        interpret(bf, &idx);
    }
    free(bf->prog);
}

/**
 * @brief Runs the brainfuck interpreter in REPL (Read-Eval-Print Loop) mode.
 *
 * This function continuously reads input from the user, appends it to the program,
 * and interprets the brainfuck instructions until the user terminates the program.
 * This allows for interactive execution of brainfuck code.
 */
void run_repl(void) {
    bf_t bf;
    char input[INPUT_MAX];
    bf->prog         = (char*) malloc(INPUT_MAX);
    size_t prog_size = INPUT_MAX;

    if (!bf->prog) {
        fprintf(stderr, "Error: Cannot allocate memory for program storage.\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        printf("> ");
        size_t program_len_old = 0;
        if (!fgets(input, INPUT_MAX, stdin)) {
            break;
        }

        program_len_old = bf->prog_len;
        bf->prog_len += strlen(input);
        if (bf->prog_len > prog_size) {
            prog_size *= 2;
            bf->prog = realloc(bf->prog, prog_size);
            if (!bf->prog) {
                fprintf(stderr, "Error: Cannot reallocate memory for program storage.\n");
                exit(EXIT_FAILURE);
            }
        }
        snprintf(bf->prog + program_len_old, INPUT_MAX - program_len_old, "%s", input);
        if (bf->prog)
            build_loops(bf->prog);

        file_index_t index;
        index.line     = 1;
        index.line_idx = 0;

        for (; bf->ip < bf->prog_len; bf->ip++) {
            interpret(bf, &index);
        }
    }

    free_brainfuck(bf);
}
