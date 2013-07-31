#
# The default target is "tg". Other targets include 
#      install: install tg binary in the appropriate platform dependent dir.
#               eg. ../bin/linux/tg 
#      clean: clean up all unnecessary files
#

PLATFORM=$(shell uname -s)
HW=$(shell uname -m)
VERS=$(shell uname -r)
IS_SUNOS = $(findstring 4., $(VERS))

ifeq ($(PLATFORM), Linux)
 CC = gcc
 CFLAGS=-g -DLINUX
 LEX=flex -l 
 YCFLAGS=-DLINUX -t 
 LIB = -ll -lm 
 INSTALL_DIR = linux
endif

ifeq ($(PLATFORM), SunOS)
 CC = gcc
ifeq ($(IS_SUNOS), )
 CFLAGS=-g -DSOLARIS
 YCFLAGS=-DSOLARIS -t 
 LEX=lex 
 LIB = -ll -lm -lsocket 
 INSTALL_DIR = solaris
else
 CFLAGS=-g -DSUNOS4
 YCFLAGS=-DSUNOS4 -t 
 LEX=flex -l
 LIB = -ll -lm -lnsl
 INSTALL_DIR = sunos
endif
endif

ifeq ($(PLATFORM), FreeBSD)
 CC = gcc
 CFLAGS=-g -DFREEBSD 
 LEX=flex -l 
 YCFLAGS=-DFREEBSD 
 LIB = -ll -lm -lcompat
 INSTALL_DIR = freebsd
endif


OBJS = \
	buffer_generic.o \
	decode.o \
	distribution.o \
	log.o \
	prot_ipport.o \
	prot_dgram.o \
	prot_stream.o \
	prot_tcp.o \
	prot_test.o \
	prot_udp.o \
	protocol.o

SRCS = ${OBJS:.o=.c}

tg :  lex.yy.c tg.o ${OBJS} Makefile 
	$(CC) -g -o tg tg.o ${OBJS} $(LIB) \

lex.yy.c : scan.l Makefile
	@echo 'Expect non-portable character class warning (ASCII assumed).'
	$(LEX) scan.l

tg.o : tg.y lex.yy.c Makefile
	@echo Expect -no- shift/reduce conflicts.
	yacc -vd tg.y
	$(CC) -g -o tg.o $(YCFLAGS) -c y.tab.c

.c.o : Makefile 
	$(CC) $(CFLAGS) -c $< 
clean	:
	rm -f tg y.tab.h y.tab.c y.output lex.yy.c *.o

lint: y.tab.c lex.yy.c
	lint y.tab.c ${SRCS} -lm

install: tg
	cp tg ../../bin/$(INSTALL_DIR)/tg


buffer_generic.o:	buffer_generic.c config.h
decode.o:	decode.c decode.h protocol.h
distribution.o:	distribution.c config.h distribution.h
log.o:		log.c decode.h log.h decode.h
prot_ipport.o:	prot_ipport.c protocol.h
prot_dgram.o:	prot_dgram.c log.h protocol.h
prot_stream.o:	prot_stream.c log.h protocol.h
prot_tcp.o:	prot_tcp.c log.h protocol.h
prot_test.o:	prot_test.c log.h protocol.h
prot_udp.o:	prot_udp.c log.h protocol.h
protocol.o:	protocol.c protocol.h
tg.o:		tg.y log.h protocol.h



