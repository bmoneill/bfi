#include "compile.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void        init_tokens(void);

static const char* tokens[']' + 1];

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
void bfx_compile(const char* input_path, const char* output_path, bfx_parameters_t params) {
    FILE* input;
    FILE* output;
    bool  binary_output = !(params.flags & BFX_FLAG_ONLY_GENERATE_C_SOURCE);
    int   c;
    int   depth;
    int   sys_ret;

    /**** Set up files ****/
    if (!input_path) {
        input = stdin;
    } else if (!(input = fopen(input_path, "r"))) {
        BFX_ERROR("Failed to open input file");
    }

    if (!output_path) {
        output_path = binary_output ? "./a.out" : "./a.out.c";
    }

    if (binary_output && !(output = fopen(BFX_TMP_FILE_PATH, "w"))) {
        BFX_ERROR("Failed to create temporary file");
    } else if (!(output = fopen(output_path, "w"))) {
        BFX_ERROR("Failed to open output file");
    }

    /*** Actual compilation ***/
    init_tokens();
    depth = 0;
    fprintf(output, BFX_COMPILE_HEAD, params.tape_size);
    while ((c = fgetc(input)) != EOF) {
        if (tokens[c]) {
            fprintf(output, "%s", tokens[c]);
        }
    }

    if (depth != 0) {
        fclose(output);
        remove(output_path);
        BFX_ERROR("Unbalanced brackets");
    }

    fprintf(output, "return 0;}");
    fclose(output);

    if (binary_output) {
        char* cmd = malloc(128);
        sprintf(cmd,
                "%s %s -o %s %s",
                BFX_DEFAULT_COMPILER,
                BFX_DEFAULT_COMPILE_FLAGS,
                output_path,
                BFX_TMP_FILE_PATH);
        sys_ret = system(cmd);
        remove(BFX_TMP_FILE_PATH);
        if (sys_ret != 0) {
            BFX_ERROR("Failed to compile program");
        }
    }
}

/**
 * @brief Initializes compiler tokens
 */
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
