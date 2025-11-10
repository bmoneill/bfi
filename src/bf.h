#ifndef BF_H
#define BF_H

#include <stddef.h>
#include <stdint.h>

#ifndef BF_DEFAULT_COMPILER
#define BF_DEFAULT_COMPILER "gcc"
#endif

#ifndef BF_DEFAULT_COMPILE_FLAGS
#define BF_DEFAULT_COMPILE_FLAGS "-O3 -s -ffast-math"
#endif

#ifndef BF_VERSION
#define BF_VERSION "unknown"
#endif

#define BF_DEFAULT_TAPE_SIZE                 30000
#define BF_DEFAULT_INPUT_MAX                 1024
#define BF_FLAG_DEBUG                        1
#define BF_FLAG_REPL                         2
#define BF_FLAG_DISABLE_SPECIAL_INSTRUCTIONS 4
#define BF_FLAG_ONLY_GENERATE_C_SOURCE       8



typedef struct {
    uint8_t flags;
    size_t  tape_size;
    size_t  input_max;
} bf_parameters_t;

void bf_compile(const char*, const char*, bf_parameters_t);
void bf_run_file(const char*, bf_parameters_t);
void bf_run_repl(bf_parameters_t);

#endif
