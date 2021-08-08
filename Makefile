CXX := g++
CC := gcc
DIR_HEADER := ./header
DIR_SOURCE := ./source
DIR_FLUTE := ./flute-3.1
CXXFLAGS := -std=c++11 -g -c -I ./header -I ./flute-3.1 -O2
CCFLAGS := -c -g -I ./flute-3.1 -O2
TARGET := main


LIBSRC += $(wildcard ${DIR_SOURCE}/*.cpp)
FLUTE_SRC += $(wildcard ${DIR_FLUTE}/*.c)
LIBOBJ = $(patsubst %.cpp,%.o,${LIBSRC})
FLUTE_OBJ = $(patsubst %.c,%.o,${FLUTE_SRC})
LIBOBJ += $(FLUTE_OBJ)


all : $(LIBOBJ) tree2tree.cpp
	${CXX} -o tree2tree.exe $^

TEST-ROUTING : $(LIBOBJ) ROUTINGTEST.cpp
	${CXX} -o ROUTINGTEST.exe $^

${DIR_FLUTE}/flute.o:${DIR_FLUTE}/flute.c ${DIR_FLUTE}/flute.h
	${CC} ${CCFLAGS} $< -o $@

${DIR_FLUTE}/dist.o:${DIR_FLUTE}/dist.c ${DIR_FLUTE}/dist.h
	${CC} ${CCFLAGS} $< -o $@

${DIR_FLUTE}/heap.o:${DIR_FLUTE}/heap.c ${DIR_FLUTE}/heap.h
	${CC} ${CCFLAGS} $< -o $@

${DIR_FLUTE}/flute_mst.o:${DIR_FLUTE}/flute_mst.c ${DIR_FLUTE}/flute.h
	${CC} ${CCFLAGS} $< -o $@

${DIR_FLUTE}/dl.o:${DIR_FLUTE}/dl.c ${DIR_FLUTE}/dl.h
	${CC} ${CCFLAGS} $< -o $@

${DIR_FLUTE}/err.o:${DIR_FLUTE}/err.c ${DIR_FLUTE}/err.h
	${CC} ${CCFLAGS} $< -o $@

${DIR_FLUTE}/neighbors.o:${DIR_FLUTE}/neighbors.c ${DIR_FLUTE}/neighbors.h
	${CC} ${CCFLAGS} $< -o $@

${DIR_FLUTE}/mst2.o:${DIR_FLUTE}/mst2.c ${DIR_FLUTE}/mst2.h
	${CC} ${CCFLAGS} $< -o $@


TEST-ROUTING-M : $(LIBOBJ) ROUTINGTEST_Monotonic.cpp
	${CXX} -o ROUTINGTEST_Monotonic.exe $^

TEST-ROUTING-L : $(LIBOBJ) ROUTINGTEST_Lshape.cpp
	${CXX} -o ROUTINGTEST_Lshape.exe $^

#之後在想一下怎麼自動檢查對應的.hpp , 就可以改成%.o :%.hpp , 但我現在還沒想到
${DIR_SOURCE}/graph.o:${DIR_SOURCE}/graph.cpp ${DIR_HEADER}/graph.hpp ${DIR_HEADER}/data_structure.hpp
	${CXX} ${CXXFLAGS} $< -o $@

${DIR_SOURCE}/data_structure.o:${DIR_SOURCE}/data_structure.cpp ${DIR_HEADER}/data_structure.hpp ${DIR_FLUTE}/flute.h
	${CXX} ${CXXFLAGS} $< -o $@

${DIR_SOURCE}/Routing.o:${DIR_SOURCE}/Routing.cpp ${DIR_HEADER}/data_structure.hpp ${DIR_HEADER}/graph.hpp
	${CXX} ${CXXFLAGS} $< -o $@

${DIR_SOURCE}/TwoPinNet.o:${DIR_SOURCE}/TwoPinNet.cpp ${DIR_HEADER}/Routing.hpp ${DIR_HEADER}/graph.hpp
	${CXX} ${CXXFLAGS} $< -o $@

.PHONY: clean
clean:
	$(RM) $(LIBOBJ)
