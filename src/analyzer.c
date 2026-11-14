/*
 * analyzer.c - Multithreaded file analyzer (OS Assignment, Topic 3)
 *
 * Counts, across a collection of text files:
 *   - total words (runs of letters/digits/underscore)
 *   - total lines
 *   - occurrences of selected event keywords (ERROR WARN INFO DEBUG FATAL)
 *
 * Modes (-m):
 *   seq    sequential baseline (one thread, no synchronization)
 *   race   threads update ONE shared counter set with NO lock  (intentionally WRONG)
 *   mutex  threads update ONE shared counter set under a mutex (correct, slow)
 *   local  each thread keeps private counts, merged once at the end (correct, fast)
 *
 * Work distribution (-d, -c):
 *   -d static   task i goes to thread (i mod T); fixed up front
 *   -d dynamic  threads grab the next unclaimed task from a shared atomic index
 *   -c BYTES    split every file into ~BYTES chunks (aligned to line boundaries)
 *               so one huge file can be shared by many threads (0 = whole files)
 *
 * Build: see Makefile.   Usage: ./analyzer -m local -t 4 -d dynamic -c 1048576 data/small
 */
#define _GNU_SOURCE
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define NKW 5
static const char *KW[NKW] = {"ERROR", "WARN", "INFO", "DEBUG", "FATAL"};
static size_t KWLEN[NKW];

/* Counters live in one array so one helper can bump any of them. */
enum { IDX_WORDS = 0, IDX_LINES = 1, IDX_KW = 2, NF = IDX_KW + NKW };
typedef struct { long v[NF]; } Counts;

typedef enum { MODE_SEQ, MODE_RACE, MODE_MUTEX, MODE_LOCAL } Mode;
typedef enum { DIST_STATIC, DIST_DYNAMIC } Dist;

typedef struct { char *path; const char *data; size_t size; } FileInfo;
typedef struct { int file; size_t start, end; } Task;   /* byte range [start,end) */

static FileInfo *files;  static int nfiles, cap_files;
static Task *tasks;      static size_t ntasks;
static size_t next_task;                 /* shared dynamic-queue index (atomic) */
static Mode mode = MODE_LOCAL;
static Dist dist = DIST_STATIC;
static int nthreads = 1;
static Counts global;                    /* shared result (race / mutex / merge target) */
static pthread_mutex_t gmtx = PTHREAD_MUTEX_INITIALIZER;   /* protects `global` */

static const char *mode_name(Mode m) {
    return m == MODE_SEQ ? "seq" : m == MODE_RACE ? "race" : m == MODE_MUTEX ? "mutex" : "local";
}

/* ---- the single place where a counter is incremented -------------------- */
static inline void incr(Counts *local, int i) {
    if (mode == MODE_RACE) {
        /* Deliberate data race: separate load and store on shared memory, no lock.
           'volatile' stops the compiler from hiding the race in a register. */
        volatile long *p = &global.v[i];
        long t = *p;
        *p = t + 1;
    } else if (mode == MODE_MUTEX) {
        pthread_mutex_lock(&gmtx);       /* lock ownership: held only for this one update */
        global.v[i]++;
        pthread_mutex_unlock(&gmtx);
    } else {
        local->v[i]++;                   /* private to the calling thread: no sharing */
    }
}

static inline int is_word(unsigned char c) { return isalnum(c) || c == '_'; }

static void scan_line(const char *p, const char *e, Counts *c) {
    incr(c, IDX_LINES);
    while (p < e) {
        while (p < e && !is_word((unsigned char)*p)) p++;
        if (p >= e) break;
        const char *s = p;
        while (p < e && is_word((unsigned char)*p)) p++;
        size_t len = (size_t)(p - s);
        incr(c, IDX_WORDS);
        for (int k = 0; k < NKW; k++)
            if (len == KWLEN[k] && memcmp(s, KW[k], len) == 0) incr(c, IDX_KW + k);
    }
}

/* Process the byte range of a task. Ranges are made line-aligned so that no line
 * is split or counted twice:  a range owns every line that STARTS inside it.      */
static void run_task(const Task *t, Counts *c) {
    const FileInfo *f = &files[t->file];
    const char *d = f->data;
    size_t size = f->size, s = t->start, e = t->end;
    if (s > 0 && d[s - 1] != '\n') {   /* we start mid-line: that line belongs to the previous range */
        const char *nl = memchr(d + s, '\n', size - s);
        if (!nl) return;
        s = (size_t)(nl - d) + 1;
    }
    size_t p = s;
    while (p < e && p < size) {
        const char *nl = memchr(d + p, '\n', size - p);
        size_t le = nl ? (size_t)(nl - d) : size; /* last line may lack '\n' */
        scan_line(d + p, d + le, c);
        p = le + 1;
    }
}

static void *worker(void *arg) {
    long id = (long)arg;
    Counts loc;
    memset(&loc, 0, sizeof loc);
    if (dist == DIST_STATIC) {
        for (size_t t = (size_t)id; t < ntasks; t += (size_t)nthreads) run_task(&tasks[t], &loc);
    } else {
        for (;;) {
            size_t t = __atomic_fetch_add(&next_task, 1, __ATOMIC_RELAXED);
            if (t >= ntasks) break;
            run_task(&tasks[t], &loc);
        }
    }
    if (mode == MODE_LOCAL) {            /* single short critical section per thread */
        pthread_mutex_lock(&gmtx);
        for (int i = 0; i < NF; i++) global.v[i] += loc.v[i];
        pthread_mutex_unlock(&gmtx);
    }
    return NULL;
}

/* ---- input handling ------------------------------------------------------ */
static int add_file(const char *path) {
    struct stat st;
    int fd = open(path, O_RDONLY);
    if (fd < 0) { fprintf(stderr, "analyzer: cannot open '%s': %s\n", path, strerror(errno)); return -1; }
    if (fstat(fd, &st) < 0 || !S_ISREG(st.st_mode)) {
        fprintf(stderr, "analyzer: '%s' is not a regular file\n", path); close(fd); return -1;
    }
    const char *data = NULL;
    if (st.st_size > 0) {
        data = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) { fprintf(stderr, "analyzer: mmap '%s': %s\n", path, strerror(errno)); close(fd); return -1; }
    }
    close(fd);                           /* mapping stays valid after close */
    if (nfiles == cap_files) {
        cap_files = cap_files ? cap_files * 2 : 64;
        files = realloc(files, (size_t)cap_files * sizeof *files);
        if (!files) { perror("realloc"); exit(1); }
    }
    files[nfiles].path = strdup(path);
    files[nfiles].data = data;
    files[nfiles].size = (size_t)st.st_size;
    nfiles++;
    return 0;
}

static int add_path(const char *path) {  /* file, or every regular file in a directory */
    struct stat st;
    if (stat(path, &st) < 0) { fprintf(stderr, "analyzer: cannot stat '%s': %s\n", path, strerror(errno)); return -1; }
    if (!S_ISDIR(st.st_mode)) return add_file(path);
    DIR *dp = opendir(path);
    if (!dp) { fprintf(stderr, "analyzer: opendir '%s': %s\n", path, strerror(errno)); return -1; }
    int rc = 0;
    struct dirent *de;
    while ((de = readdir(dp))) {
        if (de->d_name[0] == '.') continue;
        char full[4096];
        snprintf(full, sizeof full, "%s/%s", path, de->d_name);
        struct stat s2;
        if (stat(full, &s2) == 0 && S_ISREG(s2.st_mode) && add_file(full) < 0) rc = -1;
    }
    closedir(dp);
    return rc;
}

static void build_tasks(size_t chunk) {
    size_t cap = 0;
    ntasks = 0;
    for (int f = 0; f < nfiles; f++) {
        size_t sz = files[f].size;
        if (sz == 0) continue;
        size_t step = (chunk == 0 || chunk > sz) ? sz : chunk;
        for (size_t s = 0; s < sz; s += step) {
            if (ntasks == cap) {
                cap = cap ? cap * 2 : 256;
                tasks = realloc(tasks, cap * sizeof *tasks);
                if (!tasks) { perror("realloc"); exit(1); }
            }
            tasks[ntasks].file = f;
            tasks[ntasks].start = s;
            tasks[ntasks].end = (s + step < sz) ? s + step : sz;
            ntasks++;
        }
    }
}

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

static void usage(const char *p) {
    fprintf(stderr,
        "Usage: %s [-m seq|race|mutex|local] [-t THREADS] [-d static|dynamic] [-c CHUNK_BYTES] [-q] PATH...\n"
        "  PATH may be a file or a directory (regular files inside are used).\n", p);
}

int main(int argc, char **argv) {
    size_t chunk = 0;
    int quiet = 0, opt;
    for (int k = 0; k < NKW; k++) KWLEN[k] = strlen(KW[k]);
    while ((opt = getopt(argc, argv, "m:t:d:c:qh")) != -1) {
        switch (opt) {
        case 'm':
            if (!strcmp(optarg, "seq")) mode = MODE_SEQ;
            else if (!strcmp(optarg, "race")) mode = MODE_RACE;
            else if (!strcmp(optarg, "mutex")) mode = MODE_MUTEX;
            else if (!strcmp(optarg, "local")) mode = MODE_LOCAL;
            else { usage(argv[0]); return 1; }
            break;
        case 't': nthreads = atoi(optarg); break;
        case 'd':
            if (!strcmp(optarg, "static")) dist = DIST_STATIC;
            else if (!strcmp(optarg, "dynamic")) dist = DIST_DYNAMIC;
            else { usage(argv[0]); return 1; }
            break;
        case 'c': chunk = strtoull(optarg, NULL, 10); break;
        case 'q': quiet = 1; break;
        default: usage(argv[0]); return 1;
        }
    }
    if (optind >= argc) { usage(argv[0]); return 1; }
    if (nthreads < 1 || nthreads > 256) { fprintf(stderr, "analyzer: threads must be 1..256\n"); return 1; }
    if (mode == MODE_SEQ) nthreads = 1;

    int bad = 0;
    for (int i = optind; i < argc; i++) if (add_path(argv[i]) < 0) bad = 1;
    if (bad) { fprintf(stderr, "analyzer: aborting, some inputs were unreadable\n"); return 2; }

    build_tasks(chunk);
    size_t total_bytes = 0;
    for (int f = 0; f < nfiles; f++) total_bytes += files[f].size;

    double t0 = now_ms();
    if (mode == MODE_SEQ) {
        Counts loc;
        memset(&loc, 0, sizeof loc);
        for (size_t t = 0; t < ntasks; t++) run_task(&tasks[t], &loc);
        global = loc;
    } else {
        pthread_t *th = malloc((size_t)nthreads * sizeof *th);
        if (!th) { perror("malloc"); return 1; }
        int created = 0;
        for (long i = 0; i < nthreads; i++) {
            int rc = pthread_create(&th[i], NULL, worker, (void *)i);
            if (rc) { fprintf(stderr, "analyzer: pthread_create: %s\n", strerror(rc)); break; }
            created++;
        }
        for (int i = 0; i < created; i++) pthread_join(th[i], NULL);
        free(th);
        if (created != nthreads) return 3;
    }
    double t1 = now_ms();

    if (!quiet) {
        printf("files=%d tasks=%zu bytes=%zu\n", nfiles, ntasks, total_bytes);
        printf("words : %ld\nlines : %ld\n", global.v[IDX_WORDS], global.v[IDX_LINES]);
        for (int k = 0; k < NKW; k++) printf("%-6s: %ld\n", KW[k], global.v[IDX_KW + k]);
    }
    printf("RESULT mode=%s dist=%s threads=%d chunk=%zu files=%d bytes=%zu words=%ld lines=%ld",
           mode_name(mode), dist == DIST_STATIC ? "static" : "dynamic", nthreads, chunk, nfiles,
           total_bytes, global.v[IDX_WORDS], global.v[IDX_LINES]);
    for (int k = 0; k < NKW; k++) printf(" %s=%ld", KW[k], global.v[IDX_KW + k]);
    printf(" time_ms=%.3f\n", t1 - t0);

    for (int f = 0; f < nfiles; f++) {
        if (files[f].data) munmap((void *)files[f].data, files[f].size);
        free(files[f].path);
    }
    free(files); free(tasks);
    return 0;
}
