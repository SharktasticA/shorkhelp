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

install: shorkhelp
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 shorkhelp $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/shorkhelp

clean:
	rm -f shorkhelp

.PHONY: install uninstall clean
