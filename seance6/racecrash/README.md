# racecrash — data race → segfault, debugged with sanitizers

A small, deliberately broken multithreaded C program for a debugging lab.
Four producer threads append samples to a shared, dynamically-growing
array (correctly protected by a mutex). A fifth "monitor" thread computes
a rolling average by reading that same array — **without taking the
lock**, on the theory that "it's just a read, reads are safe." They
aren't: the producers' `realloc()` calls are free to move the backing
allocation and free the old block, so the monitor thread can end up
reading through a pointer that's already been freed out from under it.
Eventually that read lands on unmapped memory and the process segfaults.

This has been built and run repeatedly while putting it together — see
§4 for the actual numbers. The short version: the plain build **does**
crash, but the timing is genuinely unpredictable (anywhere from about a
second to the full 10-second run, and once in a while it doesn't crash at
all in a given run) — and that unpredictability is the whole point of the
lab, not a flaw in it.

## 1. The exercise, as given to engineers

1. Build and run the plain (`buggy`) binary a few times. Observe: it
   usually segfaults, but not at a consistent point, and not every time.
2. Try to explain the crash from the segfault alone (a core dump / gdb
   backtrace will show a crash somewhere inside `store_stats_unsafe`,
   dereferencing a pointer that "looks like" it should be valid). This is
   deliberately unsatisfying — a raw crash tells you *that* something is
   wrong, not *why*, and not reliably even *that* something is wrong,
   since plenty of runs won't crash at all despite the bug being present
   every single time.
3. Rebuild with `-fsanitize=thread` (`make tsan`). Run it. **This is the
   payoff**: ThreadSanitizer reports the exact data race — both
   conflicting accesses, with full stack traces and source lines — almost
   certainly on the very first run, because it doesn't need the race to
   escalate all the way to an actual segfault to catch it.
4. Rebuild with `-fsanitize=address` (`make asan`) for the complementary
   view: a precise `heap-use-after-free` report showing exactly which
   `realloc()` freed the memory the monitor thread went on to read.
5. Fix it: take the same lock the producers use before reading the
   store's fields (`git diff` against the `USE_LOCK_ON_READ` code path
   in `racecrash.c` if you want to check your fix against the reference
   one). Rebuild with both sanitizers (`make fixed-tsan fixed-asan`) and
   confirm they're clean.
6. (Optional, and a good discussion point once step 5 is "done".) There
   is a *second*, separate data race in this file: the `g_running`
   shutdown flag. An early draft of this program declared it
   `volatile int` and set/read it directly from multiple threads — which
   ThreadSanitizer flagged even after step 5's fix was in place, because
   it's a genuinely different bug. `volatile` in C only tells the
   compiler not to cache or reorder around a variable; it says nothing
   about cross-thread visibility or ordering and is not a synchronization
   primitive. The shipped `racecrash.c` already fixes this properly with
   `stdatomic.h` (`atomic_int` + `atomic_load_explicit`/
   `atomic_store_explicit`) — worth pointing engineers at that diff too,
   since "I marked it volatile so it's thread-safe" is a very common and
   very wrong belief, distinct from the main lock-the-read lesson.

## 2. Build

```sh
make            # builds all six variants (see targets below)
```

Or build one at a time:

| target        | binary                     | what it is                                   |
|---------------|-----------------------------|-----------------------------------------------|
| `make buggy`      | `racecrash_buggy`       | plain build, the bug is live                  |
| `make asan`       | `racecrash_asan`        | buggy code, `-fsanitize=address`              |
| `make tsan`       | `racecrash_tsan`        | buggy code, `-fsanitize=thread`               |
| `make fixed`      | `racecrash_fixed`       | `-DUSE_LOCK_ON_READ`, the fix applied         |
| `make fixed-asan` | `racecrash_fixed_asan`  | fixed code, `-fsanitize=address`              |
| `make fixed-tsan` | `racecrash_fixed_tsan`  | fixed code, `-fsanitize=thread`               |

Each run lasts 10 seconds and then exits cleanly on its own (or crashes
sooner, for the buggy variants) — no input needed.

## 3. What you'll see

**Plain buggy build**, when it crashes:
```
monitor: t=4.8s avg=49.81 count=69044
Segmentation fault (core dumped)
```
Just a crash, at some point, with no explanation of why.

**TSan build** (almost always fires within the first second or two,
whether or not the plain build would have crashed on that run):
```
WARNING: ThreadSanitizer: data race (pid=...)
  Read of size 8 at 0x... by thread T5:
    #0 store_stats_unsafe racecrash.c:92
    #1 monitor_main racecrash.c:141

  Previous write of size 8 at 0x... by thread T2 (mutexes: write M0):
    #0 store_push racecrash.c:84
    #1 producer_main racecrash.c:117
```
This alone tells you exactly which two lines conflict and that one side
holds a lock the other doesn't — enough to fix the bug without ever
needing the segfault to reproduce.

**ASan build**, on the runs where the race does escalate to an actual
use-after-free:
```
ERROR: AddressSanitizer: heap-use-after-free ...
    #0 store_stats_unsafe racecrash.c:95
freed by thread T4 here:
    #0 realloc ...
    #1 store_push racecrash.c:81
previously allocated by thread T2 here:
    #0 realloc ...
    #1 store_push racecrash.c:81
```
This is the mechanism, spelled out: which `realloc()` freed the block,
and which read touched it afterward.

**Fixed build**, under either sanitizer, repeatedly: no warnings, clean
exit every time.

## 4. Verification (numbers from actually running this)

Plain `racecrash_buggy`, 4 consecutive runs: crashed at 1.2s, 1.3s, 5.0s,
and 9.9s of wall-clock time respectively — all four crashed, but at very
different points, which is the behavior to expect and to point out to
engineers rather than something to "fix" about the lab. (Occasionally a
10-second run will complete with no crash at all — that's the same
underlying race, just not one that happened to hit an unmapped page that
time. This ties into how the crash actually happens: the array only
segfaults reliably once its `realloc()`-driven growth crosses glibc's
internal mmap threshold, at which point a moved/freed block is actually
`munmap()`-ed rather than just returned to the heap's free list — below
that threshold, a stale read typically returns silently-wrong data
instead of crashing, which is arguably worse.)

`racecrash_asan` and `racecrash_tsan`: caught the bug (heap-use-after-free
/ data race respectively) on every run tried, independent of whether that
particular run's plain-build equivalent would have segfaulted.

`racecrash_fixed`, `racecrash_fixed_asan`, `racecrash_fixed_tsan`: zero
crashes and zero sanitizer reports across multiple 10-second runs each,
after both the read-lock fix and the `g_running` atomics fix described
above.

## 5. Facilitation notes

- Sanitizer builds run noticeably slower than a plain build (TSan
  especially) — that's expected and worth mentioning up front so
  engineers don't think something else broke.
- If someone wants a live look under `gdb` instead of/alongside the
  sanitizers: `ulimit -c unlimited`, run `racecrash_buggy` until it
  crashes, then `gdb ./racecrash_buggy core` and `bt` — a fine warm-up
  before showing that TSan gets there without needing the crash at all.
- `valgrind --tool=helgrind ./racecrash_buggy` (or `--tool=drd`) is a
  reasonable substitute wherever compiler sanitizers aren't available,
  though it's noticeably slower again and the reports read differently
  from TSan's — worth a five-minute side-by-side if time allows, and a
  natural bridge into the separate `leaklab` project (see
  `../leaklab/`), which uses Valgrind's `memcheck`/`massif` instead.
