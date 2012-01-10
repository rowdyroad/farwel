name = farwel
LIBS = -lboost_regex -lboost_filesystem
SRC  = main.cpp
GCC  = g++

all:
	${GCC} -I/usr/local/include -O2 -fPIC -shared -o ${name}.so ${SRC} -L/usr/local/lib ${LIBS}
	${GCC} -O2 test.cpp -o test
debug:
	${GCC} -I/usr/local/include -fPIC -shared -o ${name}.so ${SRC} -L/usr/local/lib ${LIBS} -DDEBUG

utest:
	${GCC} -O2 test.cpp -o test