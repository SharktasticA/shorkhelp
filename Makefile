CC ?= gcc
AR ?= ar
RANLIB ?= ranlib
STRIP ?= strip

CFLAGS += -I.
LDFLAGS += -static

SRC = main.c

shorkhelp: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o shorkhelp $(LDFLAGS)
	$(STRIP) shorkhelp

PREFIX ?= /usr
BINDIR = $(PREFIX)/bin
DATDIR = $(PREFIX)/share/shorkhelp

install: shorkhelp
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 shorkhelp $(DESTDIR)$(BINDIR)

	install -d $(DESTDIR)$(DATDIR)
	install -m 644 programs.csv $(DESTDIR)$(DATDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/shorkhelp
	rm -f $(DESTDIR)$(DATDIR)/programs.csv

clean:
	rm -f shorkhelp

.PHONY: install uninstall clean
