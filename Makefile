PKGS = gtk+-3.0 webkit2gtk-4.0
CFLAGS += -flto $(shell pkg-config --cflags $(PKGS)) -pthread -O3 -Wall -Wextra -Wpedantic -Wno-unused-parameter
LDFLAGS += -flto $(shell pkg-config --libs $(PKGS)) -pthread

PREFIX ?= /usr/local
DESTDIR ?=

.PHONY: all
all: webbar webbar-stats/webbar-stats

webbar: obj/main.o obj/bar.o obj/ipc.o obj/json.o obj/web.html.o
	$(CC) $(LDFLAGS) -o $@ $^

webbar-stats/webbar-stats:
	$(MAKE) -C webbar-stats webbar-stats

obj/%.o: src/%.c src/*.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

obj/web.html.o: obj/web.html
	@mkdir -p $(@D)
	$(LD) -b binary -r -o $@ $<

obj/web.html: $(shell find web -type f)
	@mkdir -p $(@D)
	./web/index.html.sh > "$@"

.PHONY: install
install: webbar
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 webbar $(DESTDIR)$(PREFIX)/bin
	$(MAKE) -C webbar-stats install

.PHONY: clean
clean:
	rm -rf webbar obj
	$(MAKE) -C webbar-stats clean
