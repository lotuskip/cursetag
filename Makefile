# cursetag Makefile
#
# To build cursetag, you require a Curses library (such as GNU ncurses) and
# taglib. Change anything you need below. On most systems, you should just
# be able to run 'make'. This produces the executable 'cursetag' in the same
# directory.
#
VERSION="\"4\""

CPPFLAGS=-O2 -g0 -fsigned-char -DVERSION=$(VERSION)
#CPPFLAGS=-O0 -ggdb -fsigned-char -DVERSION=$(VERSION) -Wall -Wextra
CXX=g++
RM=rm -f
LDLIBS=-lncursesw -ltag

# The rest should not need to be modified:
#####################################################################

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
