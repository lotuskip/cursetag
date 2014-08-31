# cursetag Makefile
#
# To build cursetag, you require a Curses library (such as GNU ncurses) and
# taglib. Change anything you need below. You might just be able to run 'make'.
# This produces the executable 'cursetag' in the same directory.
#
VERSION="\"5\""

CPPFLAGS=-O2 -g0 
#CPPFLAGS=-O0 -ggdb -Wall -Wextra

# If you get a compile error about "ncursesw/ncurses.h" not being found,
# please comment out the following line:
CPPFLAGS += -DCOMPLICATED_CURSES_HEADER

CXX=g++
RM=rm -f
LDLIBS=-lncursesw -ltag

# The rest should not need to be modified:
#####################################################################

CPPFLAGS += -fsigned-char -DVERSION=$(VERSION)
SOURCES = src/autofill.cpp src/cursetag.cpp src/encodings.cpp \
	src/fileinfo.cpp src/filelist.cpp src/inputhandle.cpp src/io.cpp
OBJS=$(subst .cpp,.o,$(SOURCES))

all: options cursetag

options:
	@echo "used CPPFLAGS = ${CPPFLAGS}"

cursetag: $(OBJS)
	$(CXX) -o cursetag $(OBJS) $(LDLIBS)

clean:
	$(RM) $(OBJS)

src/autofill.o: src/autofill.cpp
src/cursetag.o: src/cursetag.cpp
src/encodings.o: src/encodings.cpp
src/fileinfo.o: src/fileinfo.cpp
src/filelist.o: src/filelist.cpp
src/inputhandle.o: src/inputhandle.cpp
src/io.o: src/io.cpp
