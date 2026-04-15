CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -Werror -std=c99 -O2
LDFLAGS ?=

SRC_DIR   := src
INC_DIR   := include
TEST_DIR  := tests

SRCS      := $(wildcard $(SRC_DIR)/*.c)
OBJS      := $(patsubst $(SRC_DIR)/%.c,$(SRC_DIR)/%.o,$(SRCS))

TEST_SRC  := $(TEST_DIR)/test.c
TEST_OBJ  := $(TEST_SRC:.c=.o)

TARGET    := dmst
TEST_EXE  := dmst_test

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $(OBJS) $(LDFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/dmst.h
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

test: $(TARGET) $(TEST_OBJ) $(OBJS)
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $(TEST_EXE) $(TEST_OBJ) $(OBJS) $(LDFLAGS)
	./$(TEST_EXE)

$(TEST_OBJ): $(TEST_SRC) $(INC_DIR)/dmst.h
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	rm -f $(SRC_DIR)/*.o $(TEST_DIR)/*.o $(TARGET) $(TEST_EXE)