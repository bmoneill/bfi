#ifndef BF_H
#define BF_H

#define BF_INITIAL_LOOP_SIZE        2048
#define BF_DEFAULT_TAPE_SIZE        30000
#define BF_DEFAULT_INPUT_MAX        1024
#define BF_DEFAULT_INPUT_STACK_SIZE 16
#define BF_FLAG_DEBUG               1
#define BF_FLAG_REPL                2

#define IS_REPL_MODE(b)  ((b).flags & FLAG_REPL)
#define IS_DEBUG_MODE(b) ((b).flags & FLAG_DEBUG)

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t flags;
    size_t  tape_size;
    size_t  input_max;
} bf_flags_t;

void bf_compile(const char*);
void bf_run_file(const char*, bf_flags_t);
void bf_run_repl(bf_flags_t);

#endif
