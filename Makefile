name = farwel
LIBS = -lboost_regex -lboost_filesystem -lsoci_core -lsoci_mysql -lrt
SRC  = main.cpp
GCC  = g++
INCLUDES = -I/usr/local/include -I./externals/include
LINKS = -L/usr/local/lib -L./externals/lib -L./externals/lib64

all:soci
	${GCC} -I/usr/local/include -O2 -fPIC -shared -o ${name}.so ${SRC} ${INCLUDES} ${LINKS} ${LIBS} -DNDEBUG
	${GCC} -O2 test.cpp -o test
debug:
	${GCC} -I/usr/local/include -fPIC -g -shared -o ${name}.so ${SRC} ${INCLUDES} ${LINKS} ${LIBS}
utest:
	${GCC} -O2 test.cpp -o test -g
	${GCC} -O2 fork.cpp -o fork -g

soci:
	mkdir -p externals/soci/b
	cd externals/soci/b && cmake -DCMAKE_INSTALL_PREFIX=../../ ../ && make && make install