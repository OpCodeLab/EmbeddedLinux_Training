# leaklab — heap monitoring and leak detection with Valgrind

A small program with two independent, realistic memory-leak patterns,
chosen so a single `valgrind --leak-check=full --show-leak-kinds=all` run
demonstrates most of Valgrind's leak-kind vocabulary at once, plus a
`massif` heap-profiling exercise to *see* the leak as a graph before ever
running memcheck.

## 1. The two bugs

1. **`log_push()`** keeps only the most recent `MAX_ENTRIES` (100) log
   lines in a linked list, trimming the oldest one whenever the list grows
   past that. The trim correctly unlinks the old node from the list — but
   the buggy build never frees it or its message string. Once unlinked,
   nothing points to that node any more, so Valgrind reports the node
   itself as **definitely lost**, and its `->message` string (only
   reachable *through* that lost node) as **indirectly lost**. This is a
   very common real-world shape: a "trim/rotate a linked list" bug.

2. **`cache_warm()`** mallocs 16 small strings into a global array and
   never frees them anywhere. Because the global `g_cache` array is still
   live (and still points at every block) right up to process exit,
   Valgrind reports these as **still reachable** — not "definitely lost".
   Both are leaks in the sense that the memory is never released, but
   they are genuinely different categories, and the exercise is built to
   make engineers notice the distinction rather than lump every non-freed
   byte together.

`print_heap_stats()` calls glibc's `mallinfo2()` every 200 iterations to
print live heap usage directly — "monitoring the heap" the way a
long-running embedded process might do it itself, before ever reaching
for an external tool.

## 2. Build

```sh
make            # builds both leaklab_buggy and leaklab_fixed
```

`leaklab_fixed` is built with `-DFIXED`, which frees the trimmed node
properly in `log_push()` and frees the cache and the remaining log chain
before `main()` returns.

## 3. The exercise, as given to engineers

1. Run `./leaklab_buggy` directly first (no Valgrind yet) and watch the
   `mallinfo2()` output: heap usage climbs steadily every 200 iterations
   and never comes back down. That's the leak, visible without any
   tooling at all — the point being that this is often how a leak first
   gets *noticed* in the field (a process's RSS creeping up over hours),
   long before anyone reaches for Valgrind.
2. `make massif-buggy` — profile the heap over time and look at the
   graph (`ms_print massif.buggy.out`, or open the `.out` file in
   `massif-visualizer` if installed). Compare against `make massif-fixed`:
   one climbs to the peak and keeps climbing, the other plateaus.
3. `make memcheck-buggy` — run under `memcheck` with full leak-kind
   reporting. Read the `LEAK SUMMARY` first, then go back through the
   individual loss records above it and match each one to a line in
   `leaklab.c`: which record is the trimmed `log_entry_t` nodes
   (definitely lost), which is their `->message` strings (indirectly
   lost), which is the cache (still reachable), and which is the log
   chain still linked from `g_head` at exit (also still reachable, a
   third source of the same category).
4. Fix `log_push()` so the trimmed node (and its message) get freed, and
   free the cache and the remaining chain before `main()` returns.
   `make memcheck-fixed` should now report **zero** leaks of every kind.
5. (Facilitator note, not necessarily for engineers to discover
   unassisted): getting this actually clean required an exact off-by-one
   fix in the trim/walk logic itself — an earlier draft of this file
   correctly freed *a* node on each trim but had walked one link too
   short, so it silently detached two nodes and freed only one, leaking
   exactly one node per run no matter what. Valgrind caught that
   immediately (`1 blocks are definitely lost` even in the "fixed"
   build) — a good real demonstration that "I added a free() call" isn't
   the same thing as "verified clean," and that the tool, not code
   review, is what actually confirms it.

## 4. What you'll see (numbers from actually running this)

`./leaklab_buggy` plain output, heap climbing every 200 iterations and
never dropping:
```
[iter     0] heap in use:      0.1 KB   arena:    132.0 KB
[iter   200] heap in use:     16.2 KB   arena:    132.0 KB
[iter   400] heap in use:     28.7 KB   arena:    132.0 KB
...
[iter  1800] heap in use:    118.5 KB   arena:    132.0 KB
```
`./leaklab_fixed` plain output, flat the entire run:
```
[iter     0] heap in use:      0.1 KB   arena:    132.0 KB
[iter   200] heap in use:     12.2 KB   arena:    132.0 KB
...
[iter  1800] heap in use:     12.2 KB   arena:    132.0 KB
```

`make memcheck-buggy` — `LEAK SUMMARY`, ITERATIONS=2000 and
MAX_ENTRIES=100, so exactly 1,900 trims happen over the run:
```
definitely lost: 30,400 bytes in 1,900 blocks   <- the trimmed log_entry_t structs
indirectly lost: 33,090 bytes in 1,900 blocks   <- their ->message strings
  possibly lost: 0 bytes in 0 blocks
still reachable: 4,424 bytes in 216 blocks       <- 16 cache blocks + 100 kept nodes + 100 kept messages
     suppressed: 0 bytes in 0 blocks
```

`make memcheck-fixed` — clean:
```
HEAP SUMMARY:
    in use at exit: 0 bytes in 0 blocks
  total heap usage: 4,017 allocs, 4,017 frees, 72,010 bytes allocated

All heap blocks were freed -- no leaks are possible
ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

## 5. Facilitation notes

- The `still reachable` category is worth dwelling on: it's not a bug in
  the same sense as `definitely lost` (nothing is unreachable, so there's
  no *risk* of accidentally freeing live memory twice or worse), but for
  a long-running daemon it's exactly the memory you'd want released on a
  clean shutdown path — a good bridge back to the `sensorhub-lite`
  project's `SIGTERM` handling, which does free everything it allocated
  before exiting.
- `--track-origins=yes` (already in the `memcheck-*` targets) mainly
  matters for uninitialized-value errors, which this particular program
  doesn't have — it's included so engineers see it as a standing habit
  for memcheck invocations in general, not because this lab needs it.
- Pair this with the separate `racecrash` project (`../racecrash/`) for
  contrast: that one is about a data race causing a use-after-free,
  caught with sanitizers or `helgrind`/`drd`; this one is about
  ordinary single-threaded leaks, caught with `memcheck`/`massif`. Both
  are "the bug is real and reproducible every run, but a plain execution
  either doesn't show it at all (this lab) or shows it unpredictably
  (`racecrash`) without the right tool."
