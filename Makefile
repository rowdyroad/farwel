name = farwel
connectors = db
comparers = regexp,always
comma = ,
empty =
space = ${empty} ${empty}
conns=$(shell echo ${connectors} | tr A-Z a-z | sed 's/,/ /g')
conn_factories=$(shell echo ${conns} | sed 's/\<[[:lower:]]/\U&/g')
comps=$(shell echo ${comparers} | tr A-Z a-z | sed 's/,/ /g')
comp_factories=$(shell echo ${comps} | sed 's/\<[[:lower:]]/\U&/g')
LIBS = -lboost_regex -lsoci_core -lsoci_mysql -lrt
CORE_SOURCES=$(wildcard src/*.cpp) $(addsuffix .cpp,$(addprefix src/connectors/,${conns})) $(addsuffix .cpp,$(addprefix src/comparers/,${comps}))
SRC  = farwel.cpp ${CORE_SOURCES}
CFLAGS=-O2 -fPIC -shared -Wall
INCLUDES = -iquote ./include -I/usr/local/include -I./externals/include -I/usr/include
LINKS = -L/usr/lib -L/usr/local/lib -L./externals/lib -L./externals/lib64
CC  = g++ ${INCLUDES}
all:soci connectors comparers
	${CC} -I/usr/local/include ${CFLAGS} -o ${name}.so ${SRC} ${LINKS} ${LIBS} -DNDEBUG
	${CC} -O2 test.cpp -o test
debug:
	${CC} -I/usr/local/include ${CFLAGS} -g -o ${name}.so ${SRC} ${LINKS} ${LIBS}
utest:
	${CC} -O2 test.cpp -o test -g
soci:
	mkdir -p externals/soci/b
	cd externals/soci/b && cmake -DCMAKE_INSTALL_PREFIX=../../ ../ && make && make install
connectors:
	$(shell truncate --size 0 include/connectors.h)
	$(foreach conn,${conns}, $(shell echo "#include \"connectors/${conn}.h\"" >> include/connectors.h))
	
	$(shell echo "#include \"main.h\"\nnamespace FWL { void Main::createConnectors() {" > src/connectors.cpp)
	$(foreach factory,${conn_factories}, $(shell echo "addConnector(\"${factory}\", ConnectorFactoryIntr(new ${factory}Factory, false));" >> src/connectors.cpp))
	$(shell echo "}}" >> src/connectors.cpp)
comparers:
	$(shell truncate --size 0 include/comparers.h)
	$(foreach comp,${comps}, $(shell echo "#include \"comparers/${comp}.h\"" >> include/comparers.h))
	
	$(shell echo "#include \"main.h\"\nnamespace FWL { void Main::createComparers() {" > src/comparers.cpp)
	$(foreach factory,${comp_factories}, $(shell echo "addComparer(\"${factory}\", ComparerFactoryIntr(new ${factory}Factory, false));" >> src/comparers.cpp))
	$(shell echo "}}" >> src/comparers.cpp)

clean:
	rm -f build/*
	rm -f lib/*