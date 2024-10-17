.RECIPEPREFIX:=>

# This makefile takes the following variables:
#
#	CXX       - Compiler.
#   BUILD_DIR - Base build directory.

CXXFLAGS=-O0 -ggdb -Wno-unused-but-set-variable

SOURCES_TGT=enemy.cpp entity.cpp gun.cpp player.cpp world.cpp main.cpp
HEADERS_TGT=enemy.h entity.h gun.h player.h world.h
OBJECTS_TGT=${SOURCES_TGT:.cpp=.o}

TGT=target

tgt:${TGT}
> mkdir -p ${BUILD_DIR}/tgt
> mv ${TGT} ${BUILD_DIR}/tgt

${TGT}: ${OBJECTS_TGT}
> ${CXX} ${CXXFLAGS} ${OBJECTS_TGT} ${HEADERS_TGT} \
  -o ${TGT}

${OBJECTS_LIB}: ${SOURCES_LIB} ${HEADERS_LIB}
> ${CXX} ${CXXFLAGS} -c ${SOURCES_TGT} -o ${OBJECTS_TGT}

clean_all: clean_src clean_build

clean_src:
> -rm -f *.o

clean_build:
> -rm ${BUILD_DIR}/tgt/${TGT}
