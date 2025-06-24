/*
 * @file bfi.c
 * @brief brainfuck interpreter
 * @author Ben O'Neill <ben@oneill.sh>
 *
 * @copyright Copyright (c) 2022-2025 Ben O'Neill <ben@oneill.sh>.
 * This work is released under the terms of the MIT License. See
 * LICENSE.
 */
#define MAX_LOOPS 2048
#define TAPE_SIZE 30000
#define INPUT_MAX 1024

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	uint16_t start;
	uint16_t end;
} loop_t;

bool debug = false;
bool repl = false;

char *prog;
uint8_t tape[TAPE_SIZE];
int ip = 0;
int tp = 0;
int tp_max = 0;
loop_t loops[MAX_LOOPS];
int program_len;
int num_loops;

static void build_loops(void);
static void diagnose(void);
static void interpret(void);
static int load_file(const char *path);
static void print_err(const char *s);
static void print_usage(char *argv0);
static void run_file(void);
static void run_repl(void);

int main(int argc, char *argv[]) {
	char *path = NULL;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			for (int j = 1; j < strlen(argv[i]); j++) {
				switch (argv[i][j]) {
					case 'd':
						debug = true;
						break;
					case 'r':
						repl = true;
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

	if (!repl && path) {
		load_file(path);
		run_file();
	} else if (repl && !path) {
		run_repl();
	} else {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static void build_loops(void) {
	num_loops = 0;

	for (int i = 0; i < program_len; i++) {
		if (prog[i] == '[') {
			loops[num_loops].start = i;
			num_loops++;
		} else if (prog[i] == ']') {
			for (int j = num_loops - 1; j >= 0; j--) {
				if (!loops[j].end) {
					loops[j].end = i;
					break;
				}
			}
		}
	}
}

static void diagnose() {
	fprintf(stderr, "Tape pointer: %d\nInstruction pointer: %d\n", tp, ip);

	/* print memory map */
	for (int i = 0; i < tp_max; i++) {
		fprintf(stderr, "%d: %d\n", i, tape[i]);
	}
}

static void interpret() {
	char c;
	switch (prog[ip]) {
		case '+':
			tape[tp]++;
			break;
		case '-':
			tape[tp]--;
			break;
		case '>':
			tp++;
			if (tp > tp_max) {
				tp_max = tp; // increase max memory address for debug
			}
			break;
		case '<':
			tp--;
			if (tp < 0) {
				fprintf(stderr, "tape pointer out of bounds.\n");
				tp = 0;
			}
			break;
		case ',':
			c = fgetc(stdin);
			if (c == EOF) {
				c = 0; // EOF will overflow on uint8_t
			}
			tape[tp] = c;
			break;
		case '.':
			putchar(tape[tp]);
			break;
		case '[':
			if (!tape[tp]) {
				for (int i = 0; i < num_loops; i++) {
					if (loops[i].start == ip) {
						ip = loops[i].end;
					}
				}
			}
			break;
		case ']':
			if (tape[tp]) {
				for (int i = 0; i < num_loops; i++) {
					if (loops[i].end == ip) {
						ip = loops[i].start;
					}
				}
			}
			break;
		case '#':
			if (debug) {
				diagnose();
			}
			break;
	}
}

int load_file(const char *path) {
	FILE *f = fopen(path, "r");
	if (f) {
		fseek(f, 0, SEEK_END);
		program_len = ftell(f);
		fseek(f, 0, SEEK_SET);

		prog = (char *) malloc(program_len);
		if (prog) {
			fread(prog, 1, program_len, f);
		} else {
			print_err("can't allocate memory for program storage.");
			return 1;
		}
		fclose(f);
	} else {
		print_err("can't open file.");
		return 1;
	}

	return 0;
}

static void print_err(const char *s) {
	fprintf(stderr, "error: %s\n", s);
}

static void print_usage(char *argv0) {
	fprintf(stderr, "usage: %s [-dr] [file]\n", argv0);
}

void run_repl(void) {
	char input[INPUT_MAX];
	prog = (char *) malloc(INPUT_MAX);
	size_t program_len_old = 0;

	while (1) {
		printf("> ");
		fgets(input, INPUT_MAX, stdin);
		program_len_old = program_len;
		program_len += strlen(input);
		strcpy(prog + program_len_old, input);
		build_loops();
		for (; ip < program_len; ip++) {
			interpret();
		}
	}

	free(prog);
}

void run_file(void) {
	build_loops();
	for (ip = 0; ip < program_len; ip++) {
		interpret();
	}
	free(prog);
}

