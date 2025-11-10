#include "bfi.h"

/**
 * @brief Entry point.
 */
int main(int argc, char* argv[]) {
    brainfuck_t brainfuck;
    char* path = NULL;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; j < strlen(argv[i]); j++) {
                switch (argv[i][j]) {
                case 'd':
                    brainfuck.flags |= FLAG_DEBUG;
                    break;
                case 'r':
                    brainfuck.flags |= FLAG_REPL;
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

    if (!IS_REPL_MODE(brainfuck) && path) {
        load_file(path);
        run_file(&brainfuck, );
    } else if (IS_REPL_MODE(brainfuck) && !path) {
        run_repl();
    } else {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
