/**
 * @file bfx.h
 * @brief brainfuck interpreter header
 * @author Ben O'Neill <ben@oneill.sh>
 *
 * @copyright Copyright (c) 2022-2025 Ben O'Neill <ben@oneill.sh>.
 * This work is released under the terms of the MIT License. See
 * LICENSE.
 */

#ifndef BFX_H
#define BFX_H

#include <stddef.h>
#include <stdint.h>

#ifndef BFX_DEFAULT_COMPILER
#define BFX_DEFAULT_COMPILER "gcc"
#endif

#ifndef BFX_DEFAULT_COMPILE_FLAGS
#define BFX_DEFAULT_COMPILE_FLAGS "-O3 -s -ffast-math"
#endif

#ifndef BFX_VERSION
#define BFX_VERSION "unknown"
#endif

#define BFX_EOF_BEHAVIOR_ZERO      0
#define BFX_EOF_BEHAVIOR_DECREMENT 1
#define BFX_EOF_BEHAVIOR_UNCHANGED 2

#define BFX_FLAG_DEBUG                        1
#define BFX_FLAG_REPL                         2
#define BFX_FLAG_DISABLE_SPECIAL_INSTRUCTIONS 4
#define BFX_FLAG_ONLY_GENERATE_C_SOURCE       8
#define BFX_FLAG_GRAPHICS                     16
#define BFX_FLAG_BRAINFORK                    32
#define BFX_FLAG_PBRAIN                       64
#define BFX_FLAG_GRIN                         128
#define BFX_FLAG_SEPARATE_INPUT_AND_SOURCE    256

#define BFX_DEFAULT_EOF_BEHAVIOR BF_EOF_BEHAVIOR_ZERO
#define BFX_DEFAULT_INPUT_MAX    1024
#define BFX_DEFAULT_TAPE_SIZE    30000

#define BFX_ERROR(s)                                                                               \
    fprintf(stderr, "libbfx: Error: %s\n", s);                                                     \
    exit(EXIT_FAILURE);

#define BFX_IN_DEBUG_MODE(b)                ((b).flags & BFX_FLAG_DEBUG)
#define BFX_IN_REPL_MODE(b)                 ((b).flags & BFX_FLAG_REPL)
#define BFX_SPECIAL_INSTRUCTIONS_ENABLED(b) (!((b).flags & BFX_FLAG_DISABLE_SPECIAL_INSTRUCTIONS))

/**
 * @brief Structure to represent an index in a file (or user input).
 * @param idx Index within the file or user input.
 * @param line Line number in the file or user input.
 * @param line_idx Index within the line.
 */
typedef struct {
    int idx; /* only used for build_loops() */
    int line;
    int line_idx;
} bfx_file_index_t;

/**
 *  @brief Structure to represent a loop in the brainfuck program.
 *   This structure holds the start and end indices of a loop,
 *   allowing the interpreter to efficiently jump between matching
 *   brackets during execution.
 *   @note The `start` field represents the index of the opening bracket '[',
 *         and the `end` field represents the index of the closing bracket ']'.
 */
typedef struct {
    bfx_file_index_t start;
    bfx_file_index_t end;
} bfx_loop_t;
/**
 * @brief Structure to represent a brainfuck interpreter.
 * @param flags Flags for the interpreter.
 * @param prog Pointer to the brainfuck program string.
 * @param prog_len Length of the brainfuck program string.
 * @param prog_size Size of the brainfuck program string.
 * @param tape Pointer to the tape array.
 * @param tape_size Size of the tape array.
 * @param ip Instruction pointer.
 * @param tp Data pointer.
 * @param tp_max Maximum data pointer value.
 * @param loops Pointer to the loop array.
 * @param loops_len Length of the loop array.
 * @param loops_size Size of the loop array.
 */
typedef struct {
    int         flags;
    char*       prog;
    size_t      prog_len;
    size_t      prog_size;
    uint8_t*    tape;
    size_t      tape_size;
    int         ip;
    int         tp;
    int         tp_max;
    bfx_loop_t* loops;
    size_t      loops_len;
    size_t      loops_size;
} bfx_t;

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
} bfx_parameters_t;

void bfx_reset(bfx_t*);
void bfx_run_file(const char*, bfx_parameters_t);
void bfx_run_repl(bfx_parameters_t);

#endif
