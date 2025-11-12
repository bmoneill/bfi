/**
 * @file bfi.h
 * @brief brainfuck interpreter header
 * @author Ben O'Neill <ben@oneill.sh>
 *
 * @copyright Copyright (c) 2022-2025 Ben O'Neill <ben@oneill.sh>.
 * This work is released under the terms of the MIT License. See
 * LICENSE.
 */

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

#define BF_EOF_BEHAVIOR_ZERO      0
#define BF_EOF_BEHAVIOR_DECREMENT 1
#define BF_EOF_BEHAVIOR_UNCHANGED 2

#define BF_FLAG_DEBUG                        1
#define BF_FLAG_REPL                         2
#define BF_FLAG_DISABLE_SPECIAL_INSTRUCTIONS 4
#define BF_FLAG_ONLY_GENERATE_C_SOURCE       8
#define BF_FLAG_GRAPHICS                     16
#define BF_FLAG_BRAINFORK                    32
#define BF_FLAG_PBRAIN                       64
#define BF_FLAG_GRIN                         128
#define BF_FLAG_SEPARATE_INPUT_AND_SOURCE    256

#define BF_DEFAULT_EOF_BEHAVIOR BF_EOF_BEHAVIOR_ZERO
#define BF_DEFAULT_INPUT_MAX    1024
#define BF_DEFAULT_TAPE_SIZE    30000

/**
 * @brief Structure to hold parameters for Brainfuck compilation and execution.
 * @param flags Flags to control compilation and execution behavior.
 * @param tape_size Size of the tape.
 * @param input_max User input buffer size.
 * @param graphics_start Start cell for graphical display.
 * @param graphics_end End cell for graphical display.
 */
typedef struct {
    uint16_t flags;
    size_t   tape_size;
    size_t   input_max;
    int      eof_behavior;
    int      graphics_start;
    int      graphics_end;
} bf_parameters_t;

void bf_compile(const char*, const char*, bf_parameters_t);
void bf_run_file(const char*, bf_parameters_t);
void bf_run_repl(bf_parameters_t);

#endif
