CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude -pthread
LDFLAGS = -pthread
# -lrt exists on Linux only; macOS has no librt (its functions are in libSystem)
ifneq ($(shell uname -s),Darwin)
LDFLAGS += -lrt
endif
SRC     = $(wildcard src/*.c)
OBJ     = $(patsubst src/%.c,build/%.o,$(SRC))
BIN     = campuscompute

# modules the unit tests link against (everything except main/demo/IPC code)
UNIT_OBJ = build/job.o build/scheduler.o build/banker.o

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

build/unit_tests: tests/unit_tests.c $(UNIT_OBJ)
	$(CC) $(CFLAGS) tests/unit_tests.c $(UNIT_OBJ) -o $@

unit: build/unit_tests
	./build/unit_tests

cli: $(BIN)
	bash tests/run_tests.sh

# full test suite: unit tests + command-line tests
test: unit cli

clean:
	rm -rf build $(BIN) logs/*.log

.PHONY: all clean unit cli test
