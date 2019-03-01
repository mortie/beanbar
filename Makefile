PKGS = gtk+-3.0 webkit2gtk-4.0
CFLAGS := $(shell pkg-config --cflags $(PKGS)) -g -Wall -Wextra -Wpedantic $(CFLAGS)
LDFLAGS := $(shell pkg-config --libs $(PKGS)) -lpthread $(LDFLAGS)

bar: obj/main.o obj/bar.o obj/ipc.o obj/json.o obj/web.html.o
	$(CC) $(LDFLAGS) -o $@ $^

obj/%.o: src/%.c src/*.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

obj/web.html.o: obj/web.html
	@mkdir -p $(@D)
	$(LD) -b binary -r -o $@ $<

obj/web.html: $(shell find web -type f)
	@mkdir -p $(@D)
	$(CPP) -xc -P -o $@ web/index.html

clean:
	rm -rf bar obj
