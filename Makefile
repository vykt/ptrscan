.RECIPEPREFIX:=>

#TODO [set as required] TODO
CXX=g++
CXXFLAGS=-ggdb -Wall -fPIC
INCLUDE=-lcmore -llain

PTR_DIR="./src/ptr"
TGT_DIR="./src/tgt"

SRC_DIR=$(shell pwd)/src
BUILD_DIR=$(shell pwd)/build
DOC_DIR=$(shell pwd)/doc

INSTALL_DIR=/usr/local



#[set build options]
ifeq ($(build),debug)
	CPPFLAGS += -O0 -fsanitize=address,thread
else
	CPPFLAGS += -O3 -march=native
endif


#[process targets]
all: ptrscan tgt

install:
> cp ${BUILD_DIR}/ptr/ptrscan ${INSTALL_DIR}/bin
> mkdir -pv ${INSTALL_DIR}/share/man
> cp -R ${DOC_DIR}/roff/* ${INSTALL_DIR}/share/man

install_doc:
> mkdir -pv ${INSTALL_DIR}/share/doc/ptrscan
> cp ${DOC_DIR}/md/* ${INSTALL_DIR}/share/doc/ptrscan

uninstall:
> rm -vf ${INSTALL_DIR}/bin/ptrscan ${INSTALL_DIR}/share/man/man1/ptrscan \
	     ${INSTALL_DIR}/share/doc/ptrscan/*
> rmdir ${INSTALL_DIR}/share/doc/ptrscan

tgt:
> $(MAKE) -C ${TGT_DIR} tgt CXX='${CXX}' BUILD_DIR='${BUILD_DIR}'

ptrscan:
> $(MAKE) -C ${PTR_DIR} ptrscan CXX='${CXX}' CPPFLAGS='${CXXFLAGS}' INCLUDE='${INCLUDE}' BUILD_DIR='${BUILD_DIR}'

clean:
> $(MAKE) -C ${PTR_DIR} clean_all CXX='${CXX}' CXXFLAGS='${CXXFLAGS}' BUILD_DIR='${BUILD_DIR}'
> ${MAKE} -C ${TGT_DIR} clean_all CXX='${CXX}' BUILD_DIR='${BUILD_DIR}'
