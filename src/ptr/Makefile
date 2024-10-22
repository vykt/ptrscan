.RECIPEPREFIX:=>

# This makefile takes the following variables:
#
#	CXX       - C++ compiler.
#	CXXFLAGS  - C++ compiler flags.
#   INCLUDE   - Shared objects to link.
#   BUILD_DIR - Base build directory.

SOURCES_PTR=args.cpp debug.cpp mem.cpp mem_tree.cpp serialiser.cpp thread.cpp thread_ctrl.cpp ui_term.cpp main.cpp
HEADERS_PTR=args.h debug.h mem.h mem_tree.h serialiser.h thread.h thread_ctrl.h ui_base.h ui_term.h
OBJECTS_PTR=${SOURCES_PTR:.cpp=.o}

PTR=ptrscan


WARN_OPTS :=

ptr: ${PTR}
> mkdir -p ${BUILD_DIR}/ptr
> mv ${PTR} ${BUILD_DIR}/ptr

${PTR}: ${OBJECTS_PTR}
> ${CXX} ${CXXFLAGS} -o ${PTR} ${OBJECTS_PTR} ${HEADERS_PTR} ${WARN_OPTS} ${INCLUDE}

${OBJECTS_PTR}: ${SOURCES_PTR} ${HEADERS_PTR}
> ${CXX} ${CXXFLAGS} -c ${SOURCES_PTR} ${WARN_OPTS} ${INCLUDE}

clean_all: clean_src clean_build

clean_src:
> rm -f *.o

clean_build:
> rm ${BUILD_DIR}/ptr/${PTR}
