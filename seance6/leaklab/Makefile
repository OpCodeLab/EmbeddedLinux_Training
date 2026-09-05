CC     = gcc
CFLAGS = -Wall -Wextra -O0 -g

.PHONY: all buggy fixed clean memcheck-buggy memcheck-fixed massif-buggy massif-fixed

all: buggy fixed

buggy:
	$(CC) $(CFLAGS) -o leaklab_buggy leaklab.c

fixed:
	$(CC) $(CFLAGS) -DFIXED -o leaklab_fixed leaklab.c

# Leak detection: full leak-kind breakdown (definitely/indirectly
# lost, still reachable), with origin tracking for good measure.
memcheck-buggy: buggy
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./leaklab_buggy

memcheck-fixed: fixed
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./leaklab_fixed

# Heap-over-time profiling: produces massif.out.<pid>, view with
# `ms_print massif.out.<pid>` (or massif-visualizer if installed).
massif-buggy: buggy
	valgrind --tool=massif --massif-out-file=massif.buggy.out ./leaklab_buggy
	ms_print massif.buggy.out | head -40

massif-fixed: fixed
	valgrind --tool=massif --massif-out-file=massif.fixed.out ./leaklab_fixed
	ms_print massif.fixed.out | head -40

clean:
	rm -f leaklab_buggy leaklab_fixed massif.*.out
