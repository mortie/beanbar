PKGS = dbus-1 libpulse
CFLAGS += $(shell pkg-config --cflags $(PKGS)) -I. -Wall -Wextra -Wno-main -Wno-unused-parameter
LDFLAGS += $(shell pkg-config --libs $(PKGS))

PREFIX ?= /usr/local
DESTDIR ?=

SRC = $(wildcard src/*.c providers/*.c)
HDR = $(wildcard src/*.h providers/*.h)
OBJ = $(patsubst %.c,obj/%.o,$(SRC))

webbar-stats: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

obj/%.o: %.c $(HDR)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: install
install: webbar-stats
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 webbar-stats $(DESTDIR)$(PREFIX)/bin

.PHONY: clean
clean:
	rm -rf obj
	rm -f webbar-stats
