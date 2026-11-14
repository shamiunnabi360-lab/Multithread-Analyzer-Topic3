# Viva preparation — Topic 3

## 1. What is the aim of your project?
To compare sequential and multithreaded file analysis, demonstrate a race condition, fix shared-data problems using synchronization, and evaluate performance under different thread counts and workload distributions.

## 2. What is a thread?
A thread is an execution path within a process. Threads in one process share its address space and global data, but each thread has its own stack and CPU register state.

## 3. What is POSIX pthread?
Pthreads is the POSIX thread API for creating and managing threads in C programs on Unix-like systems.

## 4. What do `pthread_create()` and `pthread_join()` do?
`pthread_create()` starts a worker thread. `pthread_join()` waits until that worker finishes.

## 5. What is a race condition?
A race condition occurs when multiple threads access shared data concurrently and the result depends on their timing or interleaving, especially when at least one access modifies the data without proper synchronization.

## 6. Explain lost update.
Two threads can both read a shared counter value of 5, both calculate 6, and both write 6. The correct result after two increments should be 7.

## 7. How does a mutex solve this?
A mutex allows only one thread at a time to enter the protected critical section. The thread locks before updating shared data and unlocks afterward.

## 8. Why can a mutex-per-increment approach be slow?
Every counter increment requires lock and unlock operations. Many threads contend for the same lock, reducing parallelism.

## 9. What is thread-local counting?
Each worker keeps its own private counters while scanning. After finishing, it merges its totals into the global result. This avoids locking on every word or line.

## 10. What is the difference between static and dynamic scheduling?
Static scheduling assigns tasks in advance. Dynamic scheduling lets workers claim the next available task while running. Dynamic scheduling can balance uneven tasks better, but task-claim coordination has overhead.

## 11. What is chunking?
A large file is divided into smaller byte ranges (tasks), allowing multiple threads to work on different parts of the same file. The implementation aligns ownership to line starts to avoid double-counting lines.

## 12. What is speedup?
Speedup = sequential execution time / parallel execution time. A value greater than 1 means the parallel run was faster; less than 1 means it was slower.

## 13. Why does adding threads not always improve performance?
Thread creation, synchronization, task scheduling, memory bandwidth limits, and the number of available CPU cores can limit or reverse gains.

## 14. What is the role of `pthread_join()` before printing totals?
It ensures all workers have completed and merged their results before the main thread reads and prints the final counters.

## 15. Why is the race demonstration not used for benchmark results?
The unsynchronized mode has a data race and therefore is intentionally incorrect/unsafe. Only correct modes should be used for meaningful performance comparisons.

## 16. What did you learn?
[Write 2–3 sentences based on what you actually observed in your experiments.]
