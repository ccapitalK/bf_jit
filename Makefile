CXX=g++
CXXFLAGS=-std=c++17 -Wall -Wextra -O3
LDFLAGS=
OBJS=src/arguments.o src/asmbuf.o src/code_generator.o src/engine.o src/interpreter.o src/ir.o src/main.o src/optimizer.o src/parser.o src/runtime.o

.PHONY: clean

bf: ${OBJS}
	${CXX} ${CXXFLAGS} ${LDFLAGS} $^ -o $@

clean:
	rm src/*.o bf
