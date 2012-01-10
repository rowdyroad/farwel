name = farwel
LIBS = -ljson -lboost_regex
SRC  = main.cpp
GCC  = g++

all:
	${GCC} -I/usr/local/include -fPIC -shared -o ${name}.so ${SRC} -L/usr/local/lib ${LIBS}
	${GCC} -O2 test.cpp -o test
debug:
	${GCC} -I/usr/local/include -fPIC -shared -o ${name}.so ${SRC} -L/usr/local/lib ${LIBS} -DDEBUG

test:
	${GCC} -O2 test.cpp -o test