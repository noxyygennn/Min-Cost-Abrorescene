CC=gcc

CSTD=-std=c11
WARN=-Wall -Wextra -Wpedantic
DBG=-g -fno-omit-frame-pointer
SAN=-fsanitize=address,undefined

INC=-Iinclude -Isrc

SRC=src/naive.c \
	src/tarjan.c \
	src/gabow.c \
	src/reachability.c

COMMON=$(CSTD) $(WARN) $(DBG)

.PHONY: all release debug test clean

all: release

release:
	$(CC) $(CSTD) -O2 -DNDEBUG \
	$(WARN) $(INC) \
	$(SRC) src/main.c \
	-o dmst

debug:
	$(CC) $(COMMON) $(INC) \
	$(SRC) src/main.c \
	-o dmst

tests_run:
	$(CC) $(COMMON) $(SAN) \
	$(INC) $(SRC) tests/test.c \
	$(SAN) -o tests_run

test: tests_run
	./tests_run

clean:
	rm -f dmst tests_run
