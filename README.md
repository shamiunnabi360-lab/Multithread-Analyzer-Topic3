# Topic 3 - Multithreaded File Analysis and Performance Evaluation

Student ID: 2403008 | Course: Operating Systems | Language: C (POSIX threads)

## What it does
`analyzer` counts words, lines, and event keywords (ERROR, WARN, INFO, DEBUG, FATAL) across a
directory of text files. One binary implements every variant needed for the assignment:

| Mode (`-m`) | Meaning | Correct? |
|---|---|---|
| `seq`   | Sequential baseline | yes |
| `race`  | Threads update one shared counter set with no lock (controlled race demo) | NO, by design |
| `mutex` | Shared counters protected by a mutex on every update | yes, slow |
| `local` | Thread-local counters merged once at the end | yes, fast |

| Distribution | Meaning |
|---|---|
| `-d static`  | task i goes to thread i mod T |
| `-d dynamic` | threads pull the next task from a shared atomic counter |
| `-c BYTES`   | split files into line-aligned chunks (challenge: handles very uneven file sizes) |

## Dependencies
gcc (or clang), make, pthreads, bash, python3. Optional: matplotlib for graphs (`pip install matplotlib`).

## Build and run
    make                 # builds ./analyzer
    make test            # runs the test suite (tests/run_tests.sh)
    make data            # generates datasets in data/ (about 400 MB; fixed seed => repeatable)
    make bench           # runs all experiments -> results/timings.csv
    python3 scripts/summarize.py   # tables + results/speedup.png + results/skewed.png
    bash scripts/env.sh > results/environment.txt   # hardware/software for the report
    make tsan
    ./analyzer_tsan -m race  -t 4 -d dynamic data/small 2>&1 | grep -c 'data race'   # reports races
    ./analyzer_tsan -m local -t 4 -d dynamic data/small 2>&1 | grep -c 'data race'   # 0

Examples:

    ./analyzer -m seq data/small
    ./analyzer -m race -t 8 data/medium          # watch the totals come out wrong
    ./analyzer -m local -t 8 -d dynamic -c 1048576 data/skewed

Run `REPS=3 THREADS="1 2 4" bash scripts/bench.sh` for a quicker benchmark.

## Limitations
- Keywords are compiled in (edit `KW[]` in `src/analyzer.c`).
- Files are memory-mapped, so the whole input must fit in the address space; the first pass over a file pays page-cache/disk cost.
- The `race` mode has undefined behaviour on purpose; never use it for real results.
- Words are ASCII letters/digits/underscore only (no Unicode word rules).



