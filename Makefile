name = farwel
LIBS = -lboost_regex -lboost_filesystem -lsoci_core -lsoci_mysql
SRC  = main.cpp
GCC  = g++
INCLUDES = -I/usr/local/include
LINKS = -L/usr/local/lib

all:
	${GCC} -I/usr/local/include -O2 -fPIC -shared -o ${name}.so ${SRC} -L/usr/local/lib ${LIBS}
	${GCC} -O2 test.cpp -o test
debug:
	${GCC} -I/usr/local/include -fPIC -g -shared -o ${name}.so ${SRC} -L/usr/local/lib ${LIBS} -DDEBUG

utest:
	${GCC} -O2 test.cpp -o test -g
	${GCC} soci_test.cpp -o soci_test ${INCLUDES} ${LINKS} -lsoci_core -lsoci_mysql