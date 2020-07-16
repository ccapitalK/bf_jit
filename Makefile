CXX=g++
CXXFLAGS=-std=c++17 -Wall -Wextra -Ilibs/cxxopts/include -O3
LDFLAGS=
OBJS=src/arguments.o src/asmbuf.o src/code_generator.o src/interpreter.o src/ir.o src/main.o src/runtime.o

.PHONY: clean

bf: ${OBJS}
	${CXX} ${CXXFLAGS} ${LDFLAGS} $^ -o $@

clean:
	rm src/*.o bf
