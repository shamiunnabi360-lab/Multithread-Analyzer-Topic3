# Multithreaded File Analysis and Performance Evaluation
**Operating Systems Assignment — Topic 3**  
**Student ID:** 2403008  
**Student name:** [Shamiun Nabi Chawdhury]  
**Course/Section:** [FILL]  
**Submission date:** [FILL]

> Replace every `[FILL ...]` item. Use only measurements from your own execution. Do not submit this template unchanged.

## Abstract
This project implements a file-analysis program in C and compares a sequential baseline with multithreaded approaches using POSIX threads. It demonstrates an unsynchronized shared-counter race, a mutex-protected solution, and a thread-local-counter solution. Experiments compare execution time under different thread counts and scheduling strategies.

## 1. Objectives
1. Implement a sequential file analyzer as a baseline.
2. Divide file-analysis work among POSIX threads.
3. Demonstrate a race condition on shared counters.
4. correct shared updates using a mutex and thread-local aggregation.
5. Measure execution time and speedup for different thread counts and input sizes.
6. Discuss synchronization overhead and workload distribution.

## 2. System and experimental environment
Paste the actual contents of `results/environment.txt`.

| Item | Actual value |
|---|---|
| CPU | [FILL from environment.txt] |
| Logical processors | [FILL] |
| RAM | [FILL] |
| OS and version | [FILL] |
| Compiler | [FILL] |
| Compiler flags | `-O2 -Wall -Wextra -std=gnu11` |
| Benchmark repetitions | [FILL actual number] |

## 3. Program design and implementation
The program reads regular text files and counts total words, lines, and occurrences of five event keywords: ERROR, WARN, INFO, DEBUG and FATAL. Input files are memory-mapped for scanning.

### 3.1 Sequential baseline
The main thread processes all tasks one by one. This gives the baseline execution time.

### 3.2 Multithreading with POSIX threads
The program creates worker threads using `pthread_create()`. Each worker processes assigned tasks. The main thread calls `pthread_join()` to wait for all workers before printing the final result.

### 3.3 Race condition
In race mode, workers update shared counters without a mutex. Two workers may read the same old value and overwrite each other's increments. This lost-update behavior makes the result unreliable. The race mode is only a demonstration and is not used for valid performance comparisons.

### 3.4 Mutex solution
In mutex mode, a worker locks the shared mutex, updates a shared counter, and unlocks it. This protects the critical section but causes frequent lock contention because every counter update needs synchronization.

### 3.5 Thread-local solution
In local mode, each worker updates private counters while processing its tasks. At the end, it briefly locks the mutex and merges its private totals into the global counters. This greatly reduces time spent in the critical section.

### 3.6 Work distribution
Static scheduling assigns tasks to workers in a fixed round-robin pattern. Dynamic scheduling uses an atomic task index so workers claim tasks as they become available. Chunking divides large files into smaller tasks and can improve load balance when file sizes are uneven.

## 4. Testing and correctness
Attach the actual test output from `results/test_output.txt` (or `results_test.txt` if you used the guide command).

| Test category | What was checked | Actual result |
|---|---|---|
| Known input | Manually calculated word/line/keyword counts | [FILL] |
| Boundary | Empty file and file without final newline | [FILL] |
| Chunk boundary | Several small chunk sizes | [FILL] |
| Parallel correctness | Sequential and parallel counts compared | [FILL] |
| Failure cases | Missing file, invalid thread count/mode | [FILL] |
| Race demo | Number of runs showing a mismatch | [FILL; may be zero] |

Explain that a race is undefined behavior: it may produce an incorrect count, but the exact outcome is not guaranteed on every run or machine.

## 5. Performance results
Use the median times and speedups in `results/summary.csv`. Include representative rows for small, medium and large datasets and thread counts 1, 2 and 4. Include both `local` and `mutex` modes.

| Dataset | Mode | Threads | Median time (ms) | Speedup |
|---|---|---:|---:|---:|
| small | sequential | 1 | [FILL] | 1.00 |
| small | local | 2 | [FILL] | [FILL] |
| small | local | 4 | [FILL] | [FILL] |
| medium | sequential | 1 | [FILL] | 1.00 |
| medium | local | 2 | [FILL] | [FILL] |
| medium | local | 4 | [FILL] | [FILL] |
| large | sequential | 1 | [FILL] | 1.00 |
| large | local | 2 | [FILL] | [FILL] |
| large | local | 4 | [FILL] | [FILL] |
| large | mutex | 4 | [FILL] | [FILL] |

**Formula:** Speedup = sequential median time / parallel median time.

Insert `results/speedup.png` here. Insert `results/skewed.png` here.

## 6. Discussion
Answer using your measured results:
1. Did thread-local mode become faster as the thread count increased? Give one measured example.
2. Was mutex mode slower than thread-local mode? Explain the cost of frequent locking.
3. Did the smallest dataset benefit from multiple threads? Discuss thread startup and scheduling overhead.
4. Did dynamic scheduling help with the skewed dataset? Explain the imbalance in static whole-file scheduling.
5. Did performance stop improving or become worse at a higher thread count? Explain using available CPU cores, memory bandwidth and overhead.

## 7. Limitations and future improvements
Current limitations include fixed built-in keywords, ASCII-oriented word detection, and memory-mapped input. Possible improvements include configurable keywords, a reusable thread pool, cache-line padding for per-thread counters, and more extensive datasets.

## 8. Conclusion
Summarize what the experiments showed about correctness, synchronization overhead, thread-local aggregation and workload distribution. Include at least two actual observations from your measurements.

## References
- POSIX Threads (pthreads) API documentation.
- Linux manual pages: `pthread_create(3)`, `pthread_join(3)`, `pthread_mutex_lock(3)`.
- Operating Systems course materials.
