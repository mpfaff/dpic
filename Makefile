# Makefile for dpic
# Type `make' to compile the sources and produce an executable file
# Type `make install' or, to install in /usr/local, type
# `make PREFIX=local install'  You might have to to precede the command
# with `sudo'.
# 
# To comple in safe mode, type 'make SAFEMODE=-DSAFE_MODE'
#
# To enable debug, type `make DEBUG=-DDDEBUG'
# Then e.g., `./dpic -1 <options> <file>.pic' writes level-1 debug information
# to file dpic.log.  Alternately, insert the line &1 in the diagram source.
#
# Linux expects "make DESTDIR=xxx PREFIX=yyy install":
DESTDIR = /usr
PREFIX = .
DEST = ${DESTDIR}/${PREFIX}/bin

# Activate debug:
# DEBUG = -DDDEBUG
# Debug with -g flag:
# DEBUG = -g -DDDEBUG

# Server operation: Use the -z option or uncomment the following to compile
# with read and write access (sh and copy) to arbitrary files disabled.
# SAFEMODE= -DSAFE_MODE

# For DJGPP compilation:
# CFLAGS += -mcpu=pentium -march=i386 -O

# MinGW
# LIBS += -lm -liberty

#--------------------------------------------------------------------------

MANDIR = $(DESTDIR)/$(PREFIX)/share/man/man1
DOCDIR = $(DESTDIR)/$(PREFIX)/share/doc/dpic

#-----------------------------------------------------------------------

DEFS=   
LIBS += -lm 

CC=gcc
CFLAGS += $(DEBUG) $(SAFEMODE) $(DEFS) -O
LDFLAGS += $(LIBS)
CPPFLAGS +=

#DATE = `date +%Y.%m.%d`
DATE=2020.06.01

#-----------------------------------------------------------------------

OBJECTS = main.o parser.o backend.o

CMPRS = awk '{ b=b" "$$0; if(length(b) > 60){print b; b=""}}; END{print b}'

TABLES = entryhp.h entrytv.h lxch.h lxhp.h lxnp.h lxtv.h lxcst.h tokens.y \
  lxvars.h

dpic: $(OBJECTS)
	@if test "$(DEBUG)" != "" ; then make test/bisonlog.sed ; fi
	$(CC) $(CFLAGS) -o dpic $(OBJECTS) $(LDFLAGS)

main.o: main.c dpic.h $(TABLES) dpic.tab.h dpic.tab.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c main.c

backend.o: backend.c ps.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c backend.c

parser.o: parscst.h lxerr.h dpic.tab.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o parser.o dpic.tab.c

dpic.tab.c dpic.tab.h: dpic.y
	bison -d dpic.y
#	bison -d --debug --verbose dpic.y
	if test "$(DEBUG)" != "" ; then sed -f produce.sed dpic.tab.c > xxx ; \
      mv xxx dpic.tab.c ; fi

dpic.y: $(TABLES) parser.w
	sed -e '/end tokens/,$$d'  parser.w > dpic.y
	cat tokens.y >> dpic.y
	sed -e '1,/start tokens/d' parser.w >> dpic.y

parscst.h: dpic.y
	bison --verbose dpic.y
	sed -e '1,/^Grammar/d' dpic.output | \
      sed -e '1,3d' -e '/^Terminals/,$$d' | awk -f mkparscst.awk > parscst.h
	sed -e '1,/^Grammar/d' dpic.output | \
      sed -e '/^Terminals/,$$d' | sed -e 's/^......//' > grammar.txt

test/bisonlog.sed: parscst.h
	mkdir -p test
	sed -e \
      's%^.define \([^ ]*\) [^0-9]*\([0-9][0-9]*\)$$%s/\\(Production(.\*,p=\2\\))/\\1=\1)/%' \
       parscst.h > test/bisonlog.sed

$(TABLES): dpic.toks
	awk -f lextables.awk dpic.toks
	sed -f lexerr.sed terminals | $(CMPRS) > lxerr.h
	rm -f terminals

install: installdpic installdocs

installdpic: dpic
	mkdir -p $(DEST)
	if test -x dpic.exe ; then \
      strip dpic.exe; install dpic.exe $(DEST) ; \
    else \
      strip dpic; install -m 755 dpic $(DEST) ; \
    fi

installdocs: dpic-doc.pdf
	mkdir -p $(DOCDIR)
	mkdir -p $(MANDIR)
	install -m 644 dpic-doc.pdf $(DOCDIR)
	install -m 644 doc/dpictools.pic $(DOCDIR)
	cat doc/dpic.1 | gzip > $(MANDIR)/dpic.1.gz

uninstall:
	rm -f $(DOCDIR)/dpic-doc.pdf
	rm -f $(DOCDIR)/dpictools.pic
	rm -f $(MANDIR)/dpic.1.gz
	rm -f $(DEST)/dpic $(DEST)/dpic.exe

clean:
	rm -f dpic dpic.exe *.o

veryclean: clean
	rm -f entr* *.list lxch.h lxcst.h lxhp.h lxmax.h lxnp.h lxtv.h lxvars.h
	rm -f lxcst.p lxerr.h
	rm -f Grammar.src parscst.h
	rm -f tabletoks tokens tokens.y tvterminals sortedtoks
	rm -f *.tab.c *.tab.h dpic.y errors
	rm -f *.o dpic.exe *stackdump *.log *.output test/bisonlog.sed
	( cd test; rm -f *.ps *.dvi *.aux *.log *stackdump )


distclean: clean
