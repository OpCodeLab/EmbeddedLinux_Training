CC      = gcc
CFLAGS  = -Wall -Wextra -O0 -g -pthread
FIXFLAG = -DUSE_LOCK_ON_READ

.PHONY: all buggy asan tsan fixed fixed-asan fixed-tsan clean run-buggy run-asan run-tsan run-fixed

all: buggy asan tsan fixed fixed-asan fixed-tsan

buggy:
	$(CC) $(CFLAGS) -o racecrash_buggy racecrash.c

asan:
	$(CC) $(CFLAGS) -fsanitize=address -o racecrash_asan racecrash.c

tsan:
	$(CC) $(CFLAGS) -fsanitize=thread -o racecrash_tsan racecrash.c

fixed:
	$(CC) $(CFLAGS) $(FIXFLAG) -o racecrash_fixed racecrash.c

fixed-asan:
	$(CC) $(CFLAGS) $(FIXFLAG) -fsanitize=address -o racecrash_fixed_asan racecrash.c

fixed-tsan:
	$(CC) $(CFLAGS) $(FIXFLAG) -fsanitize=thread -o racecrash_fixed_tsan racecrash.c

# convenience run targets
run-buggy: buggy
	./racecrash_buggy
run-asan: asan
	./racecrash_asan
run-tsan: tsan
	./racecrash_tsan
run-fixed: fixed
	./racecrash_fixed

clean:
	rm -f racecrash_buggy racecrash_asan racecrash_tsan \
	      racecrash_fixed racecrash_fixed_asan racecrash_fixed_tsan
