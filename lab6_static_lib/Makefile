CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11 -I./lib
AR       = ar
ARFLAGS  = rcs

DEBUG_FLAGS   = -O0 -g
RELEASE_FLAGS = -O2

.PHONY: all clean debug release run_debug run_release

all: release

debug:   CFLAGS += $(DEBUG_FLAGS)
release: CFLAGS += $(RELEASE_FLAGS)

debug:   main_d
release: main_r

main_d main_r: main.c lib/lib.a
	$(CC) $(CFLAGS) -o $@ main.c -Llib -l:lib.a

lib/lib.o: lib/lib.c lib/lib.h
	$(CC) $(CFLAGS) -c $< -o $@

lib/lib.a: lib/lib.o
	$(AR) $(ARFLAGS) $@ $^

run_debug: debug
	./main_d

run_release: release
	./main_r

clean:
	rm -f *.o main_d main_r lib/*.o lib/lib.a