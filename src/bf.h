#ifndef BF_H
#define BF_H

#define BF_FLAG_DEBUG               1
#define BF_FLAG_REPL                2


#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t flags;
    size_t  tape_size;
    size_t  input_max;
} bf_parameters_t;

void bf_compile(const char*);
void bf_run_file(const char*, bf_parameters_t);
void bf_run_repl(bf_parameters_t);

#endif
