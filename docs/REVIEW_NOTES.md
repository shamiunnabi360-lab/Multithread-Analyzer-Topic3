# Review notes
- Original source compiles successfully with GCC in the validation environment.
- Original automated test suite completed with 48 passed and 0 failed in the validation environment.
- The benchmark correctness signature has been strengthened to compare word count, line count and all five keyword counts, rather than only words and lines.
- The two-hour helper uses a compact deterministic data scale and 3 repetitions across 1, 2 and 4 threads.
- Timing values are intentionally not prefilled: they must come from the student's own Ubuntu laptop.
- The race demo is nondeterministic by definition. Never promise that every run will show a wrong result.
