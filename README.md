# BF_JIT

This is a simple interpreter for the Brainfuck language. It uses a 3 address code IR format
to perform some useful optimizations, which it then just in time compiles into x86_64 machine
code. As a result of this approach, this interpreter only supports the x86_64 platform. The
memory array wraps around, so programs that depend on that functionality should work.

# Build

```
$ make
```

# Usage

```
$ ./bf program.bf
```


# Roadmap

- Add cxxopts dependency, configure code generation and runtime environment based arguments
- Add support for 16bit and 32bit cells
- Add configurable buffer flushes after every print

