PKGS = dbus-1 libpulse
CFLAGS += $(shell pkg-config --cflags $(PKGS)) -I. -Wall -Wextra -Wno-main -Wno-unused-parameter
LDFLAGS += $(shell pkg-config --libs $(PKGS)) -lm

PREFIX ?= /usr/local
DESTDIR ?=

SRC = $(wildcard src/*.c providers/*.c)
HDR = $(wildcard src/*.h providers/*.h)
OBJ = $(patsubst %.c,obj/%.o,$(SRC))

beanbar-stats: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

obj/%.o: %.c $(HDR)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: install
install: beanbar-stats
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 beanbar-stats $(DESTDIR)$(PREFIX)/bin

.PHONY: clean
clean:
	rm -rf obj
	rm -f beanbar-stats
