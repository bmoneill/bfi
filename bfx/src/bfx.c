/**
 * @file bfx.c
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

#define EOF_BEHAVIOR_ZERO_S      "zero"
#define EOF_BEHAVIOR_DECREMENT_S "decrement"
#define EOF_BEHAVIOR_UNCHANGED_S "unchanged"

static int  get_eof_behavior(const char*);
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
    params.eof_behavior         = BFX_DEFAULT_EOF_BEHAVIOR;

    while ((opt = getopt(argc, argv, "cCde:g:Go:Prst:vY")) != -1) {
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
        case 'e':
            if ((params.eof_behavior = get_eof_behavior(optarg)) == -1) {
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
            break;
        case 'g':
            printf("-%c Unimplemented.\n", opt);
            break;
        case 'G':
            printf("-%c Unimplemented.\n", opt);
            break;
        case 'o':
            output_path = optarg;
            break;
        case 'P':
            printf("-%c Unimplemented.\n", opt);
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
        case 'Y':
            printf("-%c Unimplemented.\n", opt);
            break;
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

static int get_eof_behavior(const char* s) {
    int         i;
    const char* eof_behavior[3];
    eof_behavior[BFX_EOF_BEHAVIOR_ZERO]      = EOF_BEHAVIOR_ZERO_S;
    eof_behavior[BFX_EOF_BEHAVIOR_DECREMENT] = EOF_BEHAVIOR_DECREMENT_S;
    eof_behavior[BFX_EOF_BEHAVIOR_UNCHANGED] = EOF_BEHAVIOR_UNCHANGED_S;

    for (i = 0; i < 3; i++) {
        if (!strcmp(s, eof_behavior[i])) {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Prints the usage message for the program.
 *
 * @param argv0 The name of the program as it was invoked.
 */
static void print_usage(const char* argv0) {
    fprintf(stderr,
            "Usage: %s [-cCdGPrsvY] [-e eof_behavior] [-g start-end] [-o output_file] [-t "
            "tape_size] [file]\n",
            argv0);
    fprintf(stderr, "Options:\n");
    fprintf(stderr,
            " -c:\t\t\tCompile the brainfuck code into a native executable. Default output\n");
    fprintf(stderr, "    \t\t\tfile is ./a.out.\n");
    fprintf(stderr,
            " -C:\t\t\tCompile the brainfuck code into C. Default output file is ./a.out.c.\n");
    fprintf(stderr,
            " -d:\t\t\tEnable debugging mode (# will print the line number, tape pointer,\n");
    fprintf(stderr, "    \t\t\tinstruction pointer, and a memory dump.\n");
    fprintf(stderr, " -G:\t\t\tEnable Grin language support\n");
    fprintf(stderr, " -P:\t\t\tEnable pbrain language support\n");
    fprintf(stderr, " -r:\t\t\tEnable REPL mode\n");
    fprintf(stderr, " -s:\t\t\tEnable ! instruction (contents after ! will be used as input) \n");
    fprintf(stderr, " -v:\t\t\tPrint version information\n");
    fprintf(stderr, " -Y:\t\t\tEnable brainfork language support\n");
    fprintf(stderr, "\n");
    fprintf(stderr, " -e eof_behavior:\tSet the behavior of the interpreter when EOF is\n");
    fprintf(stderr,
            "                 \tencountered in the input. Valid values are \"%s\" (default),\n",
            EOF_BEHAVIOR_ZERO_S);
    fprintf(stderr,
            "                 \t\"%s\", or \"%s\".\n",
            EOF_BEHAVIOR_DECREMENT_S,
            EOF_BEHAVIOR_UNCHANGED_S);
    fprintf(stderr, " -g start-end:\t\tDisplay the contents of the tape between the specified\n");
    fprintf(stderr, "                 \tindices as pixels on a graphical display. 0 will be a\n");
    fprintf(stderr, "                 \tblack pixel, and any other value will be a white pixel.\n");
    fprintf(stderr,
            " -o output_file:\t(for compilation) Write the output to the specified file instead of "
            "a.out(.c).\n");
    fprintf(stderr,
            " -t tape_size:\tSet the size of the tape. Default is %d.\n",
            BFX_DEFAULT_TAPE_SIZE);
}

static void print_version(const char* argv0) { fprintf(stderr, "%s %s\n", argv0, BFX_VERSION); }
