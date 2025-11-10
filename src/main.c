#include "bf.h"

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

    bf_parameters_t params
        = { .flags = 0, .input_max = BF_DEFAULT_INPUT_MAX, .tape_size = BF_DEFAULT_TAPE_SIZE };

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; j < strlen(argv[i]); j++) {
                switch (argv[i][j]) {
                case 'd':
                    params.flags |= BF_FLAG_DEBUG;
                    break;
                case 'r':
                    params.flags |= BF_FLAG_REPL;
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
static void print_usage(const char* argv0) { fprintf(stderr, "usage: %s [-dr] [file]\n", argv0); }
