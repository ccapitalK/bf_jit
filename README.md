# BF_JIT

This is a simple interpreter for the Brainfuck language. It uses a 3 address code IR format
to perform some useful optimizations, which it then just in time compiles into x86_64 machine
code. As a result of this approach, this interpreter only supports the x86_64 platform. The
memory array wraps around, so programs that depend on that functionality should work.
8, 16 and 32 bit cells are supported.

# Build

```
$ make
```

# Usage

```
$ ./bf --help
Usage:
  bf_jit [OPTION...] [input files]

  -m, --mem-size arg        Number of memory cells (default: 32768)
      --file-names arg      BF source file names
  -d, --dump-code           Dump the generated machine code
      --dump-mem            Dump the first 32 cells of memory after
                            termination
  -e, --eof-behaviour arg   Behaviour on eof (one of return-0, return-255,
                            dont-modify) (default: return-0)
  -g, --gen-syms            Generate jit symbol maps for debugging purposes
  -h, --help                Print help
  -n, --no-flush            Don't flush after each character
  -w, --cell-bit-width arg  Width of cell in bits (default: 8)
      --use-interpreter     Don't jit the IR, just interpret it
  -v, --verbose             Print more information
```


# Roadmap

- Improve constant folding and const propagation
- Improve const analysis

