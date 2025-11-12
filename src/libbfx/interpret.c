#include "interpret.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

static void diagnose(bfx_t*, bfx_file_index_t*);

void        bfx_interpret(bfx_t* bf, bfx_file_index_t* index) {
    size_t i;
    char   c;
    bool   receiving;

    receiving = true;
    index->line_idx++;
    switch (bf->prog[bf->ip]) {
    case '+':
        bf->tape[bf->tp]++;
        break;
    case '-':
        bf->tape[bf->tp]--;
        break;
    case '>':
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
        break;
    case '<':
        bf->tp--;
        if (bf->tp < 0) {
            fprintf(stderr,
                    "Warning (%d,%d): Tape pointer underflow. Tape pointer set to zero.\n",
                    index->line,
                    index->line_idx);
            bf->tp = 0;
        }
        break;
    case ',':
        if (receiving) {
            c = fgetc(stdin);
            if (c == EOF) {
                c         = 0;
                receiving = false;
            }
        }
        bf->tape[bf->tp] = c;
        break;
    case '.':
        putchar(bf->tape[bf->tp]);
        break;
    case '[':
        if (!bf->tape[bf->tp]) {
            for (i = 0; i < bf->loops_len; i++) {
                if (bf->loops[i].start.idx == bf->ip) {
                    bf->ip          = bf->loops[i].end.idx;
                    index->line     = bf->loops[i].end.line;
                    index->line_idx = bf->loops[i].end.line_idx;
                }
            }
        }
        break;
    case ']':
        if (bf->tape[bf->tp]) {
            for (i = 0; i < bf->loops_len; i++) {
                if (bf->loops[i].end.idx == bf->ip) {
                    bf->ip          = bf->loops[i].start.idx;
                    index->line     = bf->loops[i].start.line;
                    index->line_idx = bf->loops[i].start.line_idx;
                }
            }
        }
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
