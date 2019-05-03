#
#
#

all : #default rule

BIN=bin
TARGET=${BIN}/mcast-1

SOURCES=mcast-1.cpp

INCLUDES=\
nstl/strings.h \
nstl/arrayopts.h \
nstl/sockets.h \
nstl/endpoints.h

${BIN} : ; mkdir -p $@

${TARGET} : ${BIN} ${SOURCES} ${INCLUDES} Makefile
	g++ -o $@ ${SOURCES}

all : ${TARGET}

clean : ; -rm -rf ${BIN}/*
