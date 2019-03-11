PKGS = dbus-1
CFLAGS += $(shell pkg-config --cflags $(PKGS)) -I. -Wall -Wextra -Wno-main -Wno-unused-parameter
LDFLAGS += $(shell pkg-config --libs $(PKGS))

SRC = $(wildcard src/*.c providers/*.c)
HDR = $(wildcard src/*.h providers/*.h)
OBJ = $(patsubst %.c,obj/%.o,$(SRC))

webbar-stats: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

obj/%.o: %.c $(HDR)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -rf obj
