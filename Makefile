CXX := /opt/rh/devtoolset-6/root/usr/bin/g++
CC := /opt/rh/devtoolset-6/root/usr/bin/gcc

# CXX := g++
# CC := gcc
DIR_HEADER := ./header
DIR_SOURCE := ./source
CXXFLAGS := -std=c++11 -g -c -I ./header -O2
#CCFLAGS := -c -g -O2
TARGET := main


LIBSRC += $(wildcard ${DIR_SOURCE}/*.cpp)
LIBOBJ = $(patsubst %.cpp,%.o,${LIBSRC})


all : $(LIBOBJ) main.cpp
	${CXX} -lpthread -o cada0030_final.exe $^


${DIR_SOURCE}/graph.o:${DIR_SOURCE}/graph.cpp ${DIR_HEADER}/graph.hpp ${DIR_HEADER}/data_structure.hpp ${DIR_HEADER}/Routing.hpp
	${CXX} ${CXXFLAGS} $< -o $@

${DIR_SOURCE}/flute.o:${DIR_SOURCE}/flute.cpp ${DIR_HEADER}/flute.h
	${CXX} ${CXXFLAGS} $< -o $@

${DIR_SOURCE}/data_structure.o:${DIR_SOURCE}/data_structure.cpp ${DIR_HEADER}/data_structure.hpp
	${CXX} ${CXXFLAGS} $< -o $@

${DIR_SOURCE}/Routing.o:${DIR_SOURCE}/Routing.cpp ${DIR_HEADER}/data_structure.hpp ${DIR_HEADER}/graph.hpp ${DIR_HEADER}/TwoPinNet.hpp
	${CXX} ${CXXFLAGS} $< -o $@

${DIR_SOURCE}/TwoPinNet.o:${DIR_SOURCE}/TwoPinNet.cpp ${DIR_HEADER}/Routing.hpp ${DIR_HEADER}/graph.hpp ${DIR_HEADER}/TwoPinNet.hpp
	${CXX} ${CXXFLAGS} $< -o $@

${DIR_SOURCE}/RoutingSchedule.o:${DIR_SOURCE}/RoutingSchedule.cpp ${DIR_HEADER}/RoutingSchedule.hpp ${DIR_HEADER}/Routing.hpp ${DIR_HEADER}/graph.hpp ${DIR_HEADER}/analysis.hpp
	${CXX} ${CXXFLAGS} $< -o $@

.PHONY: clean
clean:
	$(RM) $(LIBOBJ)
