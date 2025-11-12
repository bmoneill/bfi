/**
 * @file main.c
 * @brief brainfuck interpreter entry point
 * @author Ben O'Neill <ben@oneill.sh>
 *
 * @copyright Copyright (c) 2022-2025 Ben O'Neill <ben@oneill.sh>.
 * This work is released under the terms of the MIT License. See
 * LICENSE.
 */

#include "bfx.h"
#include "compile.h"

#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char*);
static void print_version(const char*);

/**
 * @brief Entry point.
 */
int main(int argc, char* argv[]) {
    int             opt;
    bfx_parameters_t params;
    char*           path        = NULL;
    char*           output_path = NULL;
    bool            compile     = false;
    params.flags                = 0;
    params.input_max            = BFX_DEFAULT_INPUT_MAX;
    params.tape_size            = BFX_DEFAULT_TAPE_SIZE;

    while ((opt = getopt(argc, argv, "cCdo:rst:v")) != -1) {
        switch (opt) {
        case 'c':
            compile = true;
            break;
        case 'C':
            compile = true;
            params.flags |= BFX_FLAG_ONLY_GENERATE_C_SOURCE;
            break;
        case 'd':
            params.flags |= BFX_FLAG_DEBUG;
            break;
        case 'o':
            output_path = optarg;
            break;
        case 'r':
            params.flags |= BFX_FLAG_REPL;
            break;
        case 's':
            params.flags |= BFX_FLAG_DISABLE_SPECIAL_INSTRUCTIONS;
            break;
        case 't':
            params.tape_size = atoi(optarg);
            break;
        case 'v':
            print_version(argv[0]);
            return EXIT_SUCCESS;
        default:
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (optind < argc) {
        path = argv[optind];
    }

    if (compile) {
        bfx_compile(path, output_path, params);
        return EXIT_SUCCESS;
    }

    if (!(params.flags & BFX_FLAG_REPL) && path) {
        bfx_run_file(path, params);
    } else if ((params.flags & BFX_FLAG_REPL) && !path) {
        bfx_run_repl(params);
    } else {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief Prints the usage message for the program.
 *
 * @param argv0 The name of the program as it was invoked.
 */
static void print_usage(const char* argv0) {
    fprintf(stderr, "usage: %s [-cCdrsv] [-o output_file] [-t tape_size] [file]\n", argv0);
}

static void print_version(const char* argv0) { fprintf(stderr, "%s %s\n", argv0, BFX_VERSION); }
