#!/usr/bin/env python3
"""Deterministic test-data generator (fixed seed => repeatable experiments).

Creates under OUTDIR:
  small/   ~8 MB   total, 16 equal files
  medium/  ~64 MB  total, 16 equal files
  large/   ~256 MB total, 16 equal files
  skewed/  ~64 MB  total: one file holds ~85% of the bytes, 31 tiny files share the rest
Usage: gen_data.py OUTDIR [scale]      (scale 0.1 makes everything 10x smaller)
"""
import os, random, sys

LEVELS = ["INFO"] * 6 + ["DEBUG"] * 3 + ["WARN"] * 2 + ["ERROR"] * 2 + ["FATAL"]
MODULES = ["auth", "db", "net", "cache", "api", "sched", "disk"]
WORDS = ("connection timeout retry user session request response payload queue worker "
         "latency budget socket handshake token expired invalid file opened closed "
         "memory page fault thread started finished").split()

def make_pool(rng, n=4000):
    lines = []
    for _ in range(n):
        msg = " ".join(rng.choice(WORDS) for _ in range(rng.randint(3, 9)))
        lines.append("2026-03-%02d %02d:%02d:%02d [%s] %s: %s\n" % (
            rng.randint(1, 28), rng.randint(0, 23), rng.randint(0, 59), rng.randint(0, 59),
            rng.choice(LEVELS), rng.choice(MODULES), msg))
    return lines

def write_file(path, nbytes, pool, rng):
    written = 0
    with open(path, "w") as f:
        while written < nbytes:
            block = "".join(rng.choices(pool, k=2000))
            f.write(block)
            written += len(block)

def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "data"
    scale = float(sys.argv[2]) if len(sys.argv) > 2 else 1.0
    MB = 1024 * 1024
    rng = random.Random(20260001)
    pool = make_pool(rng)
    sets = {"small": [8 * MB // 16] * 16,
            "medium": [64 * MB // 16] * 16,
            "large": [256 * MB // 16] * 16}
    total = 64 * MB
    sets["skewed"] = [int(total * 0.85)] + [int(total * 0.15 / 31)] * 31
    for name, sizes in sets.items():
        d = os.path.join(out, name)
        os.makedirs(d, exist_ok=True)
        for i, sz in enumerate(sizes):
            write_file(os.path.join(d, "app%02d.log" % i), max(1000, int(sz * scale)), pool, rng)
        print("generated", d, len(sizes), "files")

if __name__ == "__main__":
    main()
