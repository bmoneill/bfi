/**
 * @file bfx.c
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

#include "bfx.h"

#include "interpret.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void build_loops(bfx_t*);
static void free_bf(bfx_t*);
static void init_bf(bfx_t*, bfx_parameters_t);
static void init_tokens(void);
static int  load_file(bfx_t*, const char*);
static void reset(bfx_t*);
static void reset_loops(bfx_t*);

/**
 * @brief Resets the brainfuck program state.
 *
 * This function resets the program state by clearing the program buffer,
 * tape, and loop structure, and resetting the instruction pointer and tape pointer.
 */
void bfx_reset(bfx_t* bf) {
    reset_loops(bf);
    memset(bf->prog, 0, bf->prog_len * sizeof(char));
    memset(bf->tape, 0, bf->tape_size * sizeof(uint8_t));
    bf->prog_len = 0;
    bf->ip       = 0;
    bf->tp       = 0;
    bf->tp_max   = 0;
}

/**
 * @brief Runs the brainfuck program loaded from a file.
 *
 * This function builds the loop structure for the brainfuck program,
 * then iterates through the program instructions, interpreting each one
 * until the end of the program is reached.
 */
void bfx_run_file(const char* path, bfx_parameters_t params) {
    bfx_t            bf;
    bfx_file_index_t idx;

    init_bf(&bf, params);
    load_file(&bf, path);
    idx.line     = 1;
    idx.line_idx = 0;
    build_loops(&bf);

    for (bf.ip = 0; (size_t) bf.ip < bf.prog_len; bf.ip++) {
        bfx_interpret(&bf, &idx);
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
void bfx_run_repl(bfx_parameters_t params) {
    bfx_t            bf;
    bfx_file_index_t index;
    char*            input;
    size_t           prog_len_old;

    init_bf(&bf, params);

    bf.prog_size = params.input_max;

    if (!(bf.prog = (char*) malloc(bf.prog_size + 1))
        || !(input = (char*) malloc(bf.prog_size + 1))) {
        BFX_ERROR("Cannot allocate memory for program storage.");
    }

    while (1) {
        printf("> ");
        if (!fgets(input, params.input_max, stdin)) {
            break;
        }

        prog_len_old = bf.prog_len;
        bf.prog_len += strlen(input);
        if (bf.prog_len > bf.prog_size) {
            bf.prog_size *= 2;
            if (!(bf.prog = realloc(bf.prog, bf.prog_size))) {
                BFX_ERROR("Cannot reallocate memory for program storage.");
            }
        }

        snprintf(bf.prog + prog_len_old, bf.prog_size - prog_len_old, "%s", input);
        reset_loops(&bf);
        build_loops(&bf);
        index.line     = 1;
        index.line_idx = 0;
        for (; (size_t) bf.ip < bf.prog_len; bf.ip++) {
            bfx_interpret(&bf, &index);
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
static void build_loops(bfx_t* bf) {
    bfx_file_index_t* stack;
    bfx_file_index_t  start;
    int               stack_top;
    int               stack_size;
    int               line;
    int               line_idx;
    size_t            i;

    stack          = malloc(sizeof(bfx_file_index_t) * BFX_INITIAL_LOOP_SIZE);
    stack_top      = 0;
    stack_size     = BFX_INITIAL_LOOP_SIZE;
    line           = 1;
    line_idx       = 0;
    bf->loops_len  = 0;
    bf->loops_size = BFX_INITIAL_LOOP_SIZE;
    bf->loops      = malloc(sizeof(bfx_loop_t) * BFX_INITIAL_LOOP_SIZE);

    for (i = 0; i < bf->prog_len; i++) {
        line_idx++;
        if (bf->prog[i] == '[') {
            if (stack_top >= stack_size) {
                stack_size *= 2;
                stack = realloc(stack, sizeof(bfx_file_index_t) * stack_size);
            }
            stack[stack_top].idx      = i;
            stack[stack_top].line     = line;
            stack[stack_top].line_idx = line_idx;
            stack_top++;
        } else if (bf->prog[i] == ']') {
            if (stack_top <= 0) {
                fprintf(stderr,
                        "libbfx: Error (%d,%d): Unmatched closing bracket ']'.\n",
                        line,
                        line_idx);
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
        fprintf(stderr, "libbfx: Error (%d,%d): Unmatched opening bracket '['.\n", line, line_idx);
        free(stack);
        exit(EXIT_FAILURE);
    }

    free(stack);
}

/**
 * @brief Frees the memory allocated for the brainfuck program.
 * @param bf Pointer to the brainfuck program.
 */
static void free_bf(bfx_t* bf) {
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
static void init_bf(bfx_t* bf, bfx_parameters_t params) {
    memset(bf, 0, sizeof(bfx_t));
    bf->flags        = params.flags;
    bf->tape_size    = params.tape_size;
    bf->tape         = calloc(params.tape_size, sizeof(uint8_t));
    bf->receiving    = true;
    bf->eof_behavior = params.eof_behavior;
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
static int load_file(bfx_t* bf, const char* path) {
    FILE*  f;
    size_t i;

    if ((f = fopen(path, "r"))) {
        fseek(f, 0, SEEK_END);
        bf->prog_len = ftell(f);
        fseek(f, 0, SEEK_SET);

        if ((bf->prog = (char*) malloc(bf->prog_len))) {
            if (fread(bf->prog, 1, bf->prog_len, f) != (unsigned long) bf->prog_len) {
                fprintf(stderr, "Error: Cannot read file %s.\n", path);
                free(bf->prog);
                fclose(f);
                return 1;
            }
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

    if (bf->flags & BFX_FLAG_SEPARATE_INPUT_AND_SOURCE) {
        for (i = 0; i < bf->prog_len; i++) {
            if (bf->prog[i] == '!') {
                bf->input_start = i + 1;
                bf->input_ptr   = bf->input_start;
                bf->input_len   = bf->prog_len;
                bf->prog[i]     = '\0';
                bf->prog_len    = i;
            }
        }
    }

    return 0;
}

/**
 * @brief Resets the loop structure.
 *
 * This function resets the loop structure by clearing the loop buffer.
 */
static void reset_loops(bfx_t* bf) {
    memset(bf->loops, 0, bf->loops_len);
    bf->loops_len = 0;
}
