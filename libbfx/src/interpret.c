#include "interpret.h"
#include "bfx.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

static void diagnose(bfx_t*, bfx_file_index_t*);

static void op_inc_tp(bfx_t*, bfx_file_index_t*);
static void op_dec_tp(bfx_t*, bfx_file_index_t*);
static void op_inc_t(bfx_t*);
static void op_dec_t(bfx_t*);
static void op_loop_start(bfx_t*, bfx_file_index_t*);
static void op_loop_end(bfx_t*, bfx_file_index_t*);
static void op_getchar(bfx_t*);
static void op_putchar(bfx_t*);

void        bfx_interpret(bfx_t* bf, bfx_file_index_t* index) {
    size_t i;
    char   c;

    index->line_idx++;
    switch (bf->prog[bf->ip]) {
    case '+':
        op_inc_t(bf);
        break;
    case '-':
        op_dec_t(bf);
        break;
    case '>':
        op_inc_tp(bf, index);
        break;
    case '<':
        op_dec_tp(bf, index);
        break;
    case ',':
        op_getchar(bf);
        break;
    case '.':
        op_putchar(bf);
        break;
    case '[':
        op_loop_start(bf, index);
        break;
    case ']':
        op_loop_end(bf, index);
        break;
    case '#':
        if (BFX_IN_DEBUG_MODE(*bf) && BFX_SPECIAL_INSTRUCTIONS_ENABLED(*bf)) {
            diagnose(bf, index);
        }
        break;
    case '@':
        if (BFX_IN_REPL_MODE(*bf) && BFX_SPECIAL_INSTRUCTIONS_ENABLED(*bf)) {
            bfx_reset(bf);
        }
        break;
    case '\n':
        index->line++;
        index->line_idx = 0;
        break;
    }
}

/**
 * @brief Diagnoses the brainfuck program.
 *
 * This function prints the current state of the brainfuck program, including the line number,
 * tape pointer, instruction pointer, and memory map.
 */
static void diagnose(bfx_t* bf, bfx_file_index_t* idx) {
    int i;

    fprintf(stderr,
            "Line: %d,%d\nTape pointer: %d\nInstruction pointer: %d\n",
            idx->line,
            idx->line_idx,
            bf->tp,
            bf->ip);

    fprintf(stderr, "Memory map:\n");
    for (i = 0; i < bf->tp_max; i++) {
        fprintf(stderr, "%d: %d\n", i, bf->tape[i]);
    }
}

static void op_inc_tp(bfx_t* bf, bfx_file_index_t* index) {
    bf->tp++;
    if ((size_t) bf->tp > bf->tape_size) {
        fprintf(stderr,
                "Warning (%d,%d): Tape pointer overflow. Tape pointer set to zero.\n",
                index->line,
                index->line_idx);
        bf->tp = 0;
    } else if (bf->tp > bf->tp_max) {
        bf->tp_max = bf->tp;
    }
}

static void op_dec_tp(bfx_t* bf, bfx_file_index_t* index) {
    bf->tp--;
    if (bf->tp < 0) {
        fprintf(stderr,
                "Warning (%d,%d): Tape pointer underflow. Tape pointer set to zero.\n",
                index->line,
                index->line_idx);
        bf->tp = 0;
    }
}

static void op_inc_t(bfx_t* bf) { bf->tape[bf->tp]++; }

static void op_dec_t(bfx_t* bf) { bf->tape[bf->tp]--; }

static void op_loop_start(bfx_t* bf, bfx_file_index_t* index) {
    size_t i;
    if (!bf->tape[bf->tp]) {
        for (i = 0; i < bf->loops_len; i++) {
            if (bf->loops[i].start.idx == bf->ip) {
                bf->ip          = bf->loops[i].end.idx;
                index->line     = bf->loops[i].end.line;
                index->line_idx = bf->loops[i].end.line_idx;
            }
        }
    }
}

static void op_loop_end(bfx_t* bf, bfx_file_index_t* index) {
    size_t i;
    if (bf->tape[bf->tp]) {
        for (i = 0; i < bf->loops_len; i++) {
            if (bf->loops[i].end.idx == bf->ip) {
                bf->ip          = bf->loops[i].start.idx;
                index->line     = bf->loops[i].start.line;
                index->line_idx = bf->loops[i].start.line_idx;
            }
        }
    }
}

static void op_getchar(bfx_t* bf) {
    char c;
    if (bf->flags & BFX_FLAG_SEPARATE_INPUT_AND_SOURCE) {
        if (bf->input_ptr < bf->input_len) {
            c = bf->prog[bf->input_ptr];
            bf->input_ptr++;
        } else {
            c = EOF;
        }
    } else {
        c = fgetc(stdin);
    }

    if (bf->receiving) {
        if (c == EOF) {
            bf->receiving = false;
        } else {
            bf->tape[bf->tp] = c;
        }
    }

    if (!bf->receiving) {
        switch (bf->eof_behavior) {
        case BFX_EOF_BEHAVIOR_ZERO:
            bf->tape[bf->tp] = 0;
            break;
        case BFX_EOF_BEHAVIOR_DECREMENT:
            bf->tape[bf->tp]--;
            break;
        }
    }
}

static void op_putchar(bfx_t* bf) { putchar(bf->tape[bf->tp]); }
