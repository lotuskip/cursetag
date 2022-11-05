# cursetag version
VERSION = 6

PREFIX = /usr
MANPREFIX = $(PREFIX)/share/man

CFLAGS = -Os -Wall -Wextra -Wshadow -pedantic -std=c99 -D_XOPEN_SOURCE=700 -DVERSION=\"$(VERSION)\"
LDFLAGS = -lncursesw -ltag_c
