# bfi: brainfuck interpreter and compiler

[![CI status](https://github.com/bmoneill/bfi/actions/workflows/make.yml/badge.svg?branch=main)](https://github.com/bmoneill/bfi/actions/workflows/make.yml).
[![clang-format status](https://github.com/bmoneill/bfi/actions/workflows/clang-format.yml/badge.svg?branch=main)](https://github.com/bmoneill/bfi/actions/workflows/clang-format.yml)

This is an interpreter and compiler for the Turing-complete esoteric programming language
brainfuck, written in C.

## Building

### Linux

```shell
make
sudo make install
```

## Usage

```shell
bfi [-cCdrsv] [-o output_file] [-t tape_size] [file]
```

* `-c`: Compile to native binary
* `-C`: Compile to C
* `-d`: Print tape pointer and instruction pointer whenever a `#` is encountered.
* `-r`: Run in interactive REPL mode
* `-s`: Disable interpretation of special characters (`#` and `@`)

* `-o output_file`: Specify the output file (default: './a.out' for binaries, './a.out.c' for C source)
* `-t tape_size`: Specify the size of the tape (default: 30000)

If no file is specified, `bfi` will read source code from standard input.

## Screenshots

`bfi` running [sierpinski.b](https://brainfuck.org/sierpinski.b)

![bfi sierpinski](https://oneill.sh/img/bfi-sierpinski.png)

## Further Reading

* [Esolang Wiki](https://esolangs.org/wiki/Brainfuck)
* [brainfuck.org](https://brainfuck.org/)

## Bugs

If you find a bug, submit an issue, PR, or email me with a description and/or patch.

## License

Copyright (c) 2022 Ben O'Neill <ben@oneill.sh>. This work is released under the
terms of the MIT License. See [LICENSE](LICENSE) for the license terms.
