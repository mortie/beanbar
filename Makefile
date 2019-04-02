PKGS = gtk+-3.0 webkit2gtk-4.0
CFLAGS += -flto $(shell pkg-config --cflags $(PKGS)) -pthread -O3 -Wall -Wextra -Wpedantic -Wno-unused-parameter
LDFLAGS += -flto $(shell pkg-config --libs $(PKGS)) -pthread

PREFIX ?= /usr/local
DESTDIR ?=

.PHONY: all
all: beanbar beanbar-stats/beanbar-stats

beanbar: obj/main.o obj/bar.o obj/ipc.o obj/json.o obj/web.html.o
	$(CC) -o $@ $^ $(LDFLAGS)

beanbar-stats/beanbar-stats:
	$(MAKE) -C beanbar-stats beanbar-stats

obj/%.o: src/%.c src/*.h
	@mkdir -p $(@D)
	$(CC) -o $@ -c $< $(CFLAGS)

obj/web.html.o: obj/web.html
	@mkdir -p $(@D)
	$(LD) -b binary -r -o $@ $<

obj/web.html: $(shell find web -type f)
	@mkdir -p $(@D)
	./web/index.html.sh > "$@"

.PHONY: install
install: beanbar
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 beanbar $(DESTDIR)$(PREFIX)/bin
	$(MAKE) -C beanbar-stats install

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/beanbar
	$(MAKE) -C beanbar-stats uninstall

.PHONY: clean
clean:
	rm -rf beanbar obj
	$(MAKE) -C beanbar-stats clean

.PHONY: readme-toc
readme-toc:
	markdown-toc -i README.md
