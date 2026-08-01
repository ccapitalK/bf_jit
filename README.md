# BF_JIT

This is a simple JIT compiler for the Brainfuck language, using a custom machine code generator.
It uses a 3 address code IR format to perform some useful optimizations, which it then just in
time compiles into x86_64 machine code. As a result of this approach, this interpreter only
supports the x86_64 platform. The memory array wraps around, so programs that depend on that
functionality should work. 8, 16 and 32 bit cells are supported.

# Build

```
$ make
```

# Usage

```
$ ./bf --help
Usage: ./bf [OPTIONS] [input files]

JIT-compiling interpreter for brainfuck

Options:
  -m, --mem-size SIZE        Number of memory cells (default: 32768)
  -w, --cell-bit-width BITS  Width of cell in bits (8, 16, or 32, default: 8)
  -0, --no-optimize          Don't optimize the IR
  -d, --dump-code            Dump the generated machine code
      --dry-run              Compile the code, but don't run it
      --dump-mem             Dump the first 32 cells of memory after termination
  -e, --eof-behaviour MODE   Behaviour on eof (return-0, return-255, dont-modify, default: return-0)
  -g, --gen-syms             Generate jit symbol maps for debugging purposes
  -n, --no-flush             Don't flush after each character
      --use-interpreter      Don't jit the IR, just interpret it
  -v, --verbose              Print more information
  -h, --help                 Print this help message
```


# Roadmap

- Improve constant folding and const propagation
- Improve const analysis

