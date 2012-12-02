#
# The default target is "dcat". Other targets include 
#      install: install dcat binary in the appropriate platform dependent dir.
#               eg. ../bin/linux/dcat
#      clean: clean up all unnecessary files
#

PLATFORM=$(shell uname -s)
HW=$(shell uname -m)
VERS=$(shell uname -r)
IS_SUNOS = $(findstring 4., $(VERS))

ifeq ($(PLATFORM), Linux)
 INSTALL_DIR = linux
 CCFLAGS = -DLINUX
endif

ifeq ($(PLATFORM), SunOS)
ifeq ($(IS_SUNOS), )
 INSTALL_DIR = solaris
 CCFLAGS = -DSOLARIS
else
 INSTALL_DIR = sunos
 CCFLAGS = -DSUNOS4
endif
endif

ifeq ($(PLATFORM), FreeBSD)
 INSTALL_DIR = freebsd
 CCFLAGS = -DFREEBSD
endif


CC      = gcc

PGM 	= dcat

CFLAGS	= -g -I../tg $(CCFLAGS)

OBJS	= dcat.o decode.o prot_ipport.o

SRCS	= ${OBJS:.o=.c}

${PGM}: ${OBJS}
	gcc -o ${PGM} ${OBJS}

clean:
	rm -f *.o ${PGM}

install: dcat
	cp dcat ../../bin/$(INSTALL_DIR)/dcat

