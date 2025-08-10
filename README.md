# bfi: brainfuck interpreter

This is an interpreter for the Turing-complete esoteric programming language
brainfuck, written in C.

## Building

### Linux

```shell
make
sudo make install
```

## Usage

```shell
bfi [-dr] [file]
```

* `-d`: Print tape pointer and instruction pointer whenever a `#` is encountered.
* `-r`: Run in interactive REPL mode

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
