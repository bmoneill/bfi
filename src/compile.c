#include "bf.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEAD          "#include <stdio.h>\nint main(void) {unsigned char t[%ld];int p=0;"
#define TMP_FILE_PATH "/tmp/bfi.c"

#define ERROR(s) fprintf(stderr, "Error: %s\n", s); exit(EXIT_FAILURE)

static void init_tokens(void);

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
    fprintf(output, HEAD, params.tape_size);
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
