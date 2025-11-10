include config.mk

BIN=bfi

SRC=src/bf.c src/main.c

all: $(BIN)

$(BIN): $(SRC)
	$(LD) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $^

clean:
	rm -f $(wildcard src/*.o) $(BIN)

dist:
	mkdir -p bfi-$(VERSION)
	cp -rf bfi.c bf/ README.md LICENSE config.mk Makefile bfi-$(VERSION)
	tar -cf bfi-$(VERSION).tar bfi-$(VERSION)
	gzip bfi-$(VERSION).tar
	rm -rf bfi-$(VERSION)

install:
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	cp -f bfi $(DESTDIR)$(PREFIX)/bin/bfi
	chmod 755 $(DESTDIR)$(PREFIX)/bin/bfi

uninstall:
	rm -f $(DESTDIR)/$(PREFIX)/bin/bfi

.PHONY: all clean dist install uninstall
