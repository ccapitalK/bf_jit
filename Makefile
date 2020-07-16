CXX=g++
CXXFLAGS=-std=c++17 -Wall -Wextra -Ilibs/cxxopts/include -O3
LDFLAGS=
OBJS=arguments.o asmbuf.o code_generator.o ir.o main.o runtime.o

.PHONY: clean

bf: ${OBJS}
	${CXX} ${CXXFLAGS} ${LDFLAGS} $^ -o $@

clean:
	rm *.o bf
