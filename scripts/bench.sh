#!/usr/bin/env bash
# Repeatable benchmark. Output: results/timings.csv
# Usage: bash scripts/bench.sh            (REPS=5 by default; REPS=3 bash scripts/bench.sh)
BIN=./analyzer
REPS=${REPS:-5}
THREADS=${THREADS:-"1 2 4 8"}
mkdir -p results
OUT=results/timings.csv
echo "dataset,mode,dist,chunk,threads,rep,time_ms,correct" > $OUT
sigof() {
  sed -n 's/.*words=\([0-9]*\) lines=\([0-9]*\) ERROR=\([0-9]*\) WARN=\([0-9]*\) INFO=\([0-9]*\) DEBUG=\([0-9]*\) FATAL=\([0-9]*\).*/\1\/\2\/\3\/\4\/\5\/\6\/\7/p' <<<"$1"
}

run() { # dataset mode dist chunk threads
  local ds=$1 m=$2 d=$3 c=$4 t=$5
  local out=$($BIN -q -m $m -d $d -c $c -t $t data/$ds)
  local ms=$(sed -n 's/.*time_ms=\(.*\)/\1/p' <<<"$out")
  local ok=no; [ "$(sigof "$out")" == "$REF" ] && ok=yes
  echo "$ds,$m,$d,$c,$t,$rep,$ms,$ok" >> $OUT
}

for ds in small medium large skewed; do
  [ -d data/$ds ] || { echo "missing data/$ds - run: make data"; exit 1; }
  REF=$(sigof "$(./analyzer -q -m seq data/$ds)")           # warm-up + reference answer
  for rep in $(seq $REPS); do
    run $ds seq static 0 1
    for t in $THREADS; do
      run $ds local static 0 $t
      run $ds mutex static 0 $t
      run $ds local dynamic 0 $t
      run $ds local dynamic 1048576 $t
    done
  done
  echo "finished $ds"
done
echo "wrote $OUT"
