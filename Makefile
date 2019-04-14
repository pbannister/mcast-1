#
#
# 

all : #default rule

BIN=bin
TARGET=${BIN}/mcast-1
SOURCES=mcast-1.cpp

${BIN} : ; mkdir -p $@

${TARGET} : ${BIN} ${SOURCES} Makefile
	g++ -o $@ ${SOURCES}

all : ${TARGET}

clean : ; -rm -rf ${BIN}/*

