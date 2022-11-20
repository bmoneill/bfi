/*
 * brainfuck interpreter
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
	uint16_t start, end;
} Loop;

bool debug = false;
bool repl = false;

char *prog;
uint8_t tape[TAPE_SIZE];
int ip = 0, tp = 0, tp_max = 0;
Loop loops[MAX_LOOPS];

size_t program_len;
size_t num_loops;

void build_loops() {
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

void diagnose() {
	fprintf(stderr, "Tape pointer: %d\nInstruction pointer: %d\n", tp, ip);

	/* print memory map */
	for (int i = 0; i < tp_max; i++) {
		fprintf(stderr, "%d: %d\n", i, tape[i]);
	}
}

void interpret() {
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
//				exit(1);
			}
			break;
		case ',':
			char c = fgetc(stdin);
			if (c == EOF) c = 0; // EOF will overflow on uint8_t
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

int load_file(const char *path)
{
	FILE *f = fopen(path, "r");
	if (f) {
		fseek(f, 0, SEEK_END);
		program_len = ftell(f);
		fseek(f, 0, SEEK_SET);

		prog = malloc(program_len);
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

void print_err(const char *s)
{
	fprintf(stderr, "error: %s\n", s);
}

void print_usage(char *argv0)
{
	fprintf(stderr, "usage: %s [-dr] [file]\n", argv0);
}

void run_repl()
{
	char input[INPUT_MAX];
	prog = malloc(INPUT_MAX);
	size_t program_len_old = 0;
	size_t prog_alloc = INPUT_MAX;

	while (1) {
		printf("> ");
		fgets(input, INPUT_MAX, stdin);
		program_len_old = program_len;
		program_len += strlen(input);

		/* resize prog if necessary 
		while (prog_len > prog_alloc) {
			prog_alloc *= 2;
			char *newprog = malloc(prog_alloc);
			memcpy(newprog, prog, prog_len_old);
			free(prog);
			prog = newprog;
		}*/

		strcpy(prog + program_len_old, input);

		build_loops();
		for (; ip < program_len; ip++) {
			interpret(prog[ip]);
		}
	}

	free(prog);
}

void run_file()
{
	build_loops();
	for (ip = 0; ip < program_len; ip++) {
		interpret(prog[ip]);
	}
	free(prog);
}

int main(int argc, char *argv[])
{
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
