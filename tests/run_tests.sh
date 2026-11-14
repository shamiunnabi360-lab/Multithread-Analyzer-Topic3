#!/usr/bin/env bash
# Test suite for analyzer. Run from the project root:  make test
# Normal cases, boundary cases, a failure scenario, and a race-condition demonstration.
BIN=./analyzer
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
pass=0; fail=0

field() { sed -n "s/.*RESULT.* $2=\([^ ]*\).*/\1/p" <<<"$1"; }          # field "<output>" name
check() { # name expected actual
  if [ "$2" == "$3" ]; then echo "PASS  $1"; pass=$((pass+1));
  else echo "FAIL  $1   (expected '$2', got '$3')"; fail=$((fail+1)); fi; }
sig()   { # output -> "words/lines/ERROR/WARN/INFO/DEBUG/FATAL"
  echo "$(field "$1" words)/$(field "$1" lines)/$(field "$1" ERROR)/$(field "$1" WARN)/$(field "$1" INFO)/$(field "$1" DEBUG)/$(field "$1" FATAL)"; }

# ---------- 1. hand-computed expected values (normal case) ----------
printf '2026-01-01 ERROR disk_full\nINFO started\nERROR retry ERROR\n' > $TMP/known.log
# words: 5+2+3 = 10, lines 3, ERROR 3, INFO 1
for m in seq local mutex; do
  out=$($BIN -m $m -t 4 $TMP/known.log); check "known file, mode=$m" "10/3/3/0/1/0/0" "$(sig "$out")"
done

# ---------- 2. boundary cases ----------
: > $TMP/empty.log
out=$($BIN -m local -t 4 $TMP/empty.log); check "empty file" "0/0/0/0/0/0/0" "$(sig "$out")"
printf 'ERROR' > $TMP/nonl.log
out=$($BIN -m local -t 2 $TMP/nonl.log);  check "no trailing newline" "1/1/1/0/0/0/0" "$(sig "$out")"
printf 'ERRORS INFOx xWARN\n' > $TMP/sub.log
out=$($BIN -m local $TMP/sub.log);        check "keyword only as whole word" "3/1/0/0/0/0/0" "$(sig "$out")"
out=$($BIN -m local -t 16 $TMP/known.log); check "more threads than tasks" "10/3/3/0/1/0/0" "$(sig "$out")"
for c in 1 3 7 16 25; do   # chunk boundaries landing inside / on line ends
  out=$($BIN -m local -t 4 -d dynamic -c $c $TMP/known.log); check "chunk=${c} bytes keeps every line once" "10/3/3/0/1/0/0" "$(sig "$out")"
done

# ---------- 3. sequential == parallel on real-sized data ----------
DATA=${DATA:-data/small}
if [ ! -d "$DATA" ]; then python3 scripts/gen_data.py $TMP/gen 0.1 >/dev/null; DATA=$TMP/gen/small; fi
base=$(sig "$($BIN -m seq $DATA)")
for m in local mutex; do for t in 1 2 4 8; do for d in static dynamic; do for c in 0 65536; do
  out=$($BIN -m $m -t $t -d $d -c $c $DATA)
  check "parallel==sequential m=$m t=$t d=$d c=$c" "$base" "$(sig "$out")"
done; done; done; done

# ---------- 4. independent cross-check with grep ----------
ref=$(cat $DATA/* | grep -ow ERROR | wc -l)
check "ERROR count matches grep -ow" "$ref" "$($BIN -m local -t 4 $DATA | sed -n 's/.*ERROR=\([0-9]*\) .*/\1/p')"

# ---------- 5. failure scenarios ----------
$BIN -m local /nonexistent/file.log 2>/dev/null; check "missing file -> exit code 2" "2" "$?"
$BIN -m local -t 0 $TMP/known.log 2>/dev/null;   check "invalid thread count -> exit code 1" "1" "$?"
$BIN -m bogus $TMP/known.log 2>/dev/null;        check "invalid mode -> exit code 1" "1" "$?"

# ---------- 6. race condition demonstration ----------
# The unsynchronized version is nondeterministic. On a multicore machine we expect
# wrong totals in some runs. (On a single core it may never fail - that is reported, not hidden.)
wrong=0; runs=20
for i in $(seq $runs); do
  out=$($BIN -m race -t 8 $DATA); [ "$(sig "$out")" != "$base" ] && wrong=$((wrong+1))
done
echo "INFO  race mode gave a wrong total in $wrong of $runs runs (cores: $(nproc))"
echo; echo "Passed: $pass   Failed: $fail"
[ $fail -eq 0 ]
