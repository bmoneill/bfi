#include "bf.h"

#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char*);

/**
 * @brief Entry point.
 */
int main(int argc, char* argv[]) {
    char*           path = NULL;
    int             opt;

    bf_parameters_t params
        = { .flags = 0, .input_max = BF_DEFAULT_INPUT_MAX, .tape_size = BF_DEFAULT_TAPE_SIZE };

    while ((opt = getopt(argc, argv, "cdrst:")) != -1) {
        switch (opt) {
        case 'c': /* TODO */
            break;
        case 'd':
            params.flags |= BF_FLAG_DEBUG;
            break;
        case 'r':
            params.flags |= BF_FLAG_REPL;
            break;
        case 's':
            params.flags |= BF_FLAG_DISABLE_SPECIAL_INSTRUCTIONS;
            break;
        case 't':
            params.tape_size = atoi(optarg);
            break;
        default:
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (optind < argc) {
        path = argv[optind];
    }

    if (!(params.flags & BF_FLAG_REPL) && path) {
        bf_run_file(path, params);
    } else if ((params.flags & BF_FLAG_REPL) && !path) {
        bf_run_repl(params);
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
    fprintf(stderr, "usage: %s [-cdrs] [-t tapesize] [file]\n", argv0);
}
