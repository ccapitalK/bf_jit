CXX=g++
CXXFLAGS=-std=c++17
LDFLAGS=
OBJS=asmbuf.o main.o

.PHONY: clean

bf: ${OBJS}
	${CXX} ${CXXFLAGS} ${LDFLAGS} $^ -o $@

clean:
	rm *.o bf
