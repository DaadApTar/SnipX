# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -Iinclude -g
LDFLAGS := 

# Directories
SRC_DIR := src
INCLUDE_DIR := include
BUILD_DIR := build
TEST_DIR := tests

# Automatically find all source files
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_FILES))

# Test files
TEST_SRC := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJ := $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%.test.o, $(TEST_SRC))
TEST_BIN := $(BUILD_DIR)/test_runner

# Final executable
TARGET := $(BUILD_DIR)/snipx

.PHONY: all clean test

all: $(TARGET)

# Link final app
$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ===== Testing =====

test: $(TEST_BIN)
	@echo "Running tests..."
	./$(TEST_BIN)

# Link test binary
$(TEST_BIN): $(OBJ_FILES) $(TEST_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Compile test files
$(BUILD_DIR)/%.test.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ===== Clean =====

clean:
	rm -rf $(BUILD_DIR)
