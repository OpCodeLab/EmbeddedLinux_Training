CC = gcc
CFLAGS = -Wall -I./lib
AR       = ar
ARFLAGS  = rcs

.PHONY: build clean 


build_debug: main.o lib.o
	$(CC) $(CFLAGS) -O0 -g -o main_debug main.o lib.o

build_release: main.o lib.o
	$(CC) $(CFLAGS) -O2 -o main_release main.o lib.o

clean:
	rm -f *.o
	rm ./lib/*.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

lib.o: lib/lib.c
	$(CC) $(CFLAGS) -c lib/lib.c

lib.a: lib.o
	$(AR) $(ARFLAGS) $@ $^

lib2.o: lib/lib.c lib/lib.h
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

#shared libarary
lib.so: lib2.o
	$(CC) -shared -o $@ $^

run_debug: build_debug
	./main_debug

run_release: build_release
	./main_release
