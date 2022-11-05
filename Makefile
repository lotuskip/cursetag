# cursetag - terminal audio file metadata editor
.POSIX:

include config.mk

SRC = autofill.c cursetag.c utils.c fileinfo.c filelist.c io.c
OBJ = $(SRC:.c=.o)

all: options cursetag

options:
	@echo cursetag build options:
	@echo "CFLAGS  = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "CC      = $(CC)"

config.h:
	cp config.def.h config.h

.c.o:
	$(CC) $(CFLAGS) -c $<

$(OBJ): config.h config.mk

cursetag: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f cursetag $(OBJ) cursetag-$(VERSION).tar.gz

dist: clean
	mkdir -p cursetag-$(VERSION)
	cp -R README Makefile config.mk config.def.h cursetag.1\
		 fileinfo.h filelist.h encodings.h autofill.h io.h $(SRC)\
		cursetag-$(VERSION)
	tar -cf - cursetag-$(VERSION) | gzip > cursetag-$(VERSION).tar.gz
	rm -rf cursetag-$(VERSION)

install: cursetag
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f cursetag $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/cursetag
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/cursetag.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/cursetag
	rm -f $(DESTDIR)$(MANPREFIX)/man1/cursetag.1

.PHONY: all options clean dist install uninstall
