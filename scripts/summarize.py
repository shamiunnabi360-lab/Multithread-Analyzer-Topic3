#!/usr/bin/env python3
"""Reads results/timings.csv, prints median times + speedup tables, draws graphs."""
import csv, statistics, collections, os
rows = list(csv.DictReader(open("results/timings.csv")))
g = collections.defaultdict(list)
for r in rows:
    g[(r["dataset"], r["mode"], r["dist"], int(r["chunk"]), int(r["threads"]))].append(float(r["time_ms"]))
med = {k: statistics.median(v) for k, v in g.items()}
bad = [r for r in rows if r["correct"] != "yes"]
print("Incorrect results in %d of %d runs" % (len(bad), len(rows)))
configs = sorted({(k[1], k[2], k[3]) for k in med if k[1] != "seq"})
datasets = ["small", "medium", "large", "skewed"]
lines = ["dataset,config,threads,median_ms,speedup"]
for ds in datasets:
    base = med.get((ds, "seq", "static", 0, 1))
    if base is None: continue
    print("\n== %s (sequential median: %.1f ms) ==" % (ds, base))
    for cfg in configs:
        for t in sorted({k[4] for k in med if k[0] == ds and (k[1], k[2], k[3]) == cfg}):
            m = med[(ds,) + cfg + (t,)]
            name = "%s/%s/chunk=%d" % cfg
            print("  %-28s T=%-2d %9.1f ms   speedup %.2fx" % (name, t, m, base / m))
            lines.append("%s,%s,%d,%.3f,%.3f" % (ds, name, t, m, base / m))
open("results/summary.csv", "w").write("\n".join(lines) + "\n")
try:
    import matplotlib; matplotlib.use("Agg")
    import matplotlib.pyplot as plt
except ImportError:
    print("\nmatplotlib not installed: pip install matplotlib   (then rerun for graphs)"); raise SystemExit
fig, axes = plt.subplots(1, 3, figsize=(15, 4.2))
for ax, ds in zip(axes, ["small", "medium", "large"]):
    base = med[(ds, "seq", "static", 0, 1)]
    for cfg in [("local", "static", 0), ("mutex", "static", 0)]:
        ts = sorted(k[4] for k in med if k[0] == ds and (k[1], k[2], k[3]) == cfg)
        ax.plot(ts, [base / med[(ds,) + cfg + (t,)] for t in ts], marker="o", label=cfg[0] + " (thread-local)" if cfg[0] == "local" else "mutex per update")
    ts = sorted({k[4] for k in med if k[0] == ds})
    ax.plot(ts, ts, "k--", alpha=.4, label="ideal"); ax.axhline(1, color="gray", lw=.5)
    ax.set_title("%s input" % ds); ax.set_xlabel("threads"); ax.set_ylabel("speedup vs sequential"); ax.legend()
plt.tight_layout(); plt.savefig("results/speedup.png", dpi=150)
fig, ax = plt.subplots(figsize=(6.5, 4.2))
base = med[("skewed", "seq", "static", 0, 1)]
for cfg, lab in [(("local", "static", 0), "static, whole files"), (("local", "dynamic", 0), "dynamic, whole files"),
                 (("local", "dynamic", 1048576), "dynamic, 1 MiB chunks")]:
    ts = sorted(k[4] for k in med if k[0] == "skewed" and (k[1], k[2], k[3]) == cfg)
    ax.plot(ts, [base / med[("skewed",) + cfg + (t,)] for t in ts], marker="o", label=lab)
ax.set_title("Skewed input: workload distribution"); ax.set_xlabel("threads"); ax.set_ylabel("speedup vs sequential"); ax.legend()
plt.tight_layout(); plt.savefig("results/skewed.png", dpi=150)
print("\nwrote results/speedup.png and results/skewed.png")
