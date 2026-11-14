#!/usr/bin/env bash
# Two-hour guided workflow. Run from the project root: bash scripts/run_2hour.sh
set -euo pipefail

echo "== 1/6: Check tools =="
command -v gcc >/dev/null || { echo "GCC missing. Run: sudo apt update && sudo apt install build-essential"; exit 1; }
command -v python3 >/dev/null || { echo "Python 3 missing. Install python3."; exit 1; }
gcc --version | head -n 1
nproc

echo "== 2/6: Build =="
make clean
make

echo "== 3/6: Generate compact, repeatable datasets (about 20 MB total) =="
python3 scripts/gen_data.py data 0.05

echo "== 4/6: Automated correctness tests =="
mkdir -p results
bash scripts/env.sh | tee results/environment.txt
make test | tee results/test_output.txt

echo "== 5/6: Benchmark (3 repetitions; 1, 2, 4 threads) =="
REPS=3 THREADS="1 2 4" bash scripts/bench.sh

echo "== 6/6: Summary and graphs =="
python3 scripts/summarize.py

echo
echo "DONE. Check these files:"
echo "  results/environment.txt"
echo "  results/test_output.txt"
echo "  results/timings.csv"
echo "  results/summary.csv"
echo "  results/speedup.png"
echo "  results/skewed.png"
echo
echo "Use your real results in docs/REPORT_TEMPLATE.md. Do not invent timings."
