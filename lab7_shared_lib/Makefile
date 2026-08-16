CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11 -I./lib
LDFLAGS  = -Llib -l:lib.so -Wl,-rpath,'$$ORIGIN/lib'

DEBUG_FLAGS   = -O0 -g
RELEASE_FLAGS = -O2

.PHONY: all clean debug release

all: release

debug:   CFLAGS += $(DEBUG_FLAGS)
release: CFLAGS += $(RELEASE_FLAGS)

debug:   main_d
release: main_r

main_d main_r: main.c lib/lib.so
	$(CC) $(CFLAGS) -o $@ main.c $(LDFLAGS)

lib/lib.o: lib/lib.c lib/lib.h
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

lib/lib.so: lib/lib.o
	$(CC) -shared -o $@ $^

clean:
	rm -f *.o main_d main_r lib/*.o lib/lib.so