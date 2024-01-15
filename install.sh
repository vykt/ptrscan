#!/bin/sh

INSTALL_DIR=/usr/local/bin
MAN_DIR=/usr/local/share/man

#Check installation
#If not running as root:
if [ "$EUID" -ne 0 ]; then
	echo "Please run the install script as root."
	exit 1
fi

#If program has not been built:
if [ ! -f "build/scan" ]; then
	echo "'scan' executable missing in the build directory. Did you remember to run 'make' first?"
	exit 1
fi

#If install directory doesn't exist:
if [ ! -d "$INSTALL_DIR" ]; then
	echo "${INSTALL_DIR} doesn't exist. Create it or change the install directory."
	exit 1
fi

#If man directory doesn't exist:
if [ ! -d "$MAN_DIR" ]; then
	echo "${MAN_DIR} doesn't exist. Create it or change the man directory."
	exit 1
fi

#copy scan (to /usr/local/bin by default)
cp ./build/scan ${INSTALL_DIR}/ptrscan

#copy manpages (to /usr/share/man by default)
cp ./man/man1/ptrscan.1.bz2 ${MAN_DIR}/man1
