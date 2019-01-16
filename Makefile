PKGS = gtk+-3.0 webkit2gtk-4.0
CFLAGS := -g $(shell pkg-config --cflags $(PKGS)) -Wall -Wextra -Wpedantic $(CFLAGS)
LDFLAGS := $(shell pkg-config --libs $(PKGS)) $(LDFLAGS)

bar: bar.c web.html.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

web.html: $(shell find web -type f)
	$(CPP) -xc -P -o $@ web/index.html

web.html.o: web.html
	$(LD) -b binary -r -o $@ $<

clean:
	rm -f bar web.html web.html.o
