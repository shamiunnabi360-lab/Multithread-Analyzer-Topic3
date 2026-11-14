CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra -std=gnu11
LDLIBS  = -lpthread

all: analyzer

analyzer: src/analyzer.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

# ThreadSanitizer build: proves 'local'/'mutex' are race-free and 'race' is not
tsan: src/analyzer.c
	$(CC) -O1 -g -fsanitize=thread -o analyzer_tsan $< $(LDLIBS)

test: analyzer
	bash tests/run_tests.sh

data:
	python3 scripts/gen_data.py data

bench: analyzer data
	bash scripts/bench.sh

clean:
	rm -f analyzer analyzer_tsan

.PHONY: all tsan test data bench clean
