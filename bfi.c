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
 *  @brief Structure to represent a loop in the brainfuck program.
 *   This structure holds the start and end indices of a loop,
 *   allowing the interpreter to efficiently jump between matching
 *   brackets during execution.
 *   @note The `start` field represents the index of the opening bracket '[',
 *         and the `end` field represents the index of the closing bracket ']'.
 */
typedef struct {
    uint16_t start;
    uint16_t end;
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
static void diagnose(void);
static void interpret(void);
static int  load_file(const char* path);
static void print_err(const char* s);
static void print_usage(const char* argv0);
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
    int stack[MAX_LOOPS];
    int stack_top = 0;

    for (int i = 0; i < program_len; i++) {
        if (prog[i] == '[') {
            if (stack_top >= MAX_LOOPS) {
                print_err("too many nested loops.");
                exit(EXIT_FAILURE);
            }
            stack[stack_top++] = i;
        } else if (prog[i] == ']') {
            if (stack_top <= 0) {
                print_err("unmatched closing bracket ']'.");
                exit(EXIT_FAILURE);
            }
            int start = stack[--stack_top];
            if (num_loops >= MAX_LOOPS) {
                print_err("too many loops.");
                exit(EXIT_FAILURE);
            }
            loops[num_loops].start = start;
            loops[num_loops].end   = i;
            num_loops++;
        }
    }
    if (stack_top != 0) {
        print_err("unmatched opening bracket '['.");
        exit(EXIT_FAILURE);
    }
}
static void diagnose() {
    fprintf(stderr, "Tape pointer: %d\nInstruction pointer: %d\n", tp, ip);

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
static void interpret(void) {
    char c;
    bool receiving = true;

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
            fprintf(stderr, "Tape pointer overflow.\n");
            tp = 0;
        } else if (tp > tp_max) {
            tp_max = tp;
        }
        break;
    case '<':
        tp--;
        if (tp < 0) {
            fprintf(stderr, "Tape pointer underflow.\n");
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
                if (loops[i].start == ip) {
                    ip = loops[i].end;
                }
            }
        }
        break;
    case ']':
        if (tape[tp]) {
            for (int i = 0; i < num_loops; i++) {
                if (loops[i].end == ip) {
                    ip = loops[i].start;
                }
            }
        }
        break;
    case '#':
        if (debug) {
            diagnose();
        }
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
            print_err("can't allocate memory for program storage.");
            fclose(f);
            return 1;
        }
        fclose(f);
    } else {
        print_err("can't open file.");
        return 1;
    }

    return 0;
}

/**
 * @brief Prints an error message to stderr.
 * @param s String to print to stderr.
 */
static void print_err(const char* s) { fprintf(stderr, "error: %s\n", s); }

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
    build_loops();
    for (ip = 0; ip < program_len; ip++) {
        interpret();
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
        print_err("can't allocate memory for program storage.");
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

        for (; ip < program_len; ip++) {
            interpret();
        }
    }

    free(prog);
}
