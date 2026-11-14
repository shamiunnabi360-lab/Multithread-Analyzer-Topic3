# Two-hour completion guide — Student ID 2403008

## Before starting
Use Ubuntu dual boot. Open Terminal and install the tools:

```bash
sudo apt update
sudo apt install build-essential make python3 python3-matplotlib unzip
```

Extract this ZIP. In Terminal, `cd` into the `os-topic3` folder.

## 0–15 minutes: build and confirm
```bash
make clean
make
./analyzer -m seq data 2>/dev/null
```
The last command is only a usage/input check if no data folder exists; the important check is that `make` finishes without errors.

## 15–35 minutes: generate data and run tests
```bash
python3 scripts/gen_data.py data 0.05
make test | tee results_test.txt
```
Expected automated test summary: `Passed: 48 Failed: 0`. If your output differs, stop and investigate; do not claim all tests passed.

## 35–75 minutes: run experiments
```bash
bash scripts/env.sh | tee results/environment.txt
REPS=3 THREADS="1 2 4" bash scripts/bench.sh
```
This runs a compact set of repeatable experiments. The benchmark compares sequential, mutex and thread-local approaches, with static/dynamic scheduling variants.

## 75–95 minutes: make tables and graphs
```bash
python3 scripts/summarize.py
```
Expected outputs:
- `results/timings.csv` — every individual measurement
- `results/summary.csv` — medians and speedups
- `results/speedup.png` — speedup comparison
- `results/skewed.png` — load-balancing comparison

If Matplotlib is unavailable:
```bash
sudo apt install python3-matplotlib
python3 scripts/summarize.py
```

## 95–120 minutes: report and evidence
1. Open `docs/REPORT_TEMPLATE.md` in LibreOffice Writer or VS Code.
2. Replace every `[FILL ...]` placeholder with your own values from `results/`.
3. Insert the two generated PNG graphs.
4. Add screenshots of your actual terminal showing `make test`, benchmark completion, and graph files.
5. Export as PDF.
6. Keep the source folder and report together for submission.

## Important
- Never copy made-up timing values. Different CPUs and storage produce different results.
- If 3 repetitions take too long, use `REPS=2 THREADS="1 2 4"` and clearly state that in the report.
- The intentionally unsafe `race` mode demonstrates a data race. Its wrong result is not guaranteed on every machine; a correct-looking result does not mean the code is safe.
- Do not include `race` timings in performance comparisons.
- Report your real CPU, core count, RAM, Ubuntu version and compiler version from `results/environment.txt`.
