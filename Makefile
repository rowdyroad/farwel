name = farwel
LIBS = -lboost_regex -lsoci_core -lsoci_mysql -lrt
CORE_SOURCES=$(wildcard src/*.cpp) $(wildcard src/connectors/*.cpp) $(wildcard src/comparers/*.cpp)
SRC  = farwel.cpp ${CORE_SOURCES}
CFLAGS=-O2 -fPIC -shared -Wall
INCLUDES = -iquote ./include -I/usr/local/include -I./externals/include -I/usr/include
LINKS = -L/usr/lib -L/usr/local/lib -L./externals/lib -L./externals/lib64
CC  = g++ ${INCLUDES}

all:soci
	${CC} -I/usr/local/include ${CFLAGS} -o ${name}.so ${SRC} ${LINKS} ${LIBS} -DNDEBUG
	${CC} -O2 test.cpp -o test
debug:
	${CC} -I/usr/local/include ${CFLAGS} -g -o ${name}.so ${SRC} ${LINKS} ${LIBS}
utest:
	${CC} -O2 test.cpp -o test -g
soci:
	mkdir -p externals/soci/b
	cd externals/soci/b && cmake -DCMAKE_INSTALL_PREFIX=../../ ../ && make && make install

clean:
	rm -f build/*
	rm -f lib/*