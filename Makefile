# Compiler and flags
CC ?= gcc
CFLAGS = -Wall -Wextra -Iinclude -ggdb
LDFLAGS ?=
LDLIBS := -lX11 -lXinerama -lXext -lpulse -lpulse -lpthread

SENDER ?= true

# Directories
SRC_DIR := src
INCLUDE_DIR := include
BUILD_DIR := build
TEST_DIR := tests
SNIPX_SENDER_DIR = ./tools/snipx-sender

# Source and object files
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_FILES))

# Exclude main.o from test builds
TEST_OBJ_SRC := $(filter-out $(SRC_DIR)/main.c, $(SRC_FILES))
TEST_OBJ := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(TEST_OBJ_SRC))
TEST_SRC := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJ += $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%.test.o, $(TEST_SRC))
TEST_BIN := $(BUILD_DIR)/test_runner

PREFIX ?= $(HOME)/.local
INSTALL_DIR := $(PREFIX)/bin/

BUILD_CONFIG := .build-config

TARGET_NAME := snipx
SENDER_TARGET_NAME := snipx-sender

# Final executable
TARGET := $(BUILD_DIR)/$(TARGET_NAME)
ifeq ($(SENDER), true)
SENDER_TARGET := $(BUILD_DIR)/$(SENDER_TARGET_NAME)
else
CFLAGS += -DDISABLE_SENDER
endif

.PHONY: all clean test

all: $(TARGET) $(SENDER_TARGET)

# Link final app
$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)
	@echo "SENDER=$(SENDER)" > $(BUILD_CONFIG)

$(SENDER_TARGET):
ifeq ($(SENDER), true)
	cd $(SNIPX_SENDER_DIR) && go build -o ../../$(BUILD_DIR)
endif

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ===== Testing =====

test: $(TEST_BIN)
	@echo "Running tests..."
	@echo $(TEST_SRC)
	ENV_TEST1=Hello ENV_TEST2=123 ./$(TEST_BIN)

# Link test binary
$(TEST_BIN): $(TEST_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

# Compile test files
$(BUILD_DIR)/%.test.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(BUILD_CONFIG)

# ===== Install =====
ifeq ($(SENDER), true)
install: $(TARGET) $(SENDER_TARGET)
else
install: $(TARGET)
endif
	mkdir -p $(INSTALL_DIR)
	cp $(TARGET) $(INSTALL_DIR)
ifeq ($(SENDER), true)
	cp $(SENDER_TARGET) $(INSTALL_DIR)
endif
	@echo "Installed to $(INSTALL_DIR)"

uninstall:
	rm $(INSTALL_DIR)/$(TARGET_NAME) $(INSTALL_DIR)/$(SENDER_TARGET_NAME)

# ===== Clean =====

clean:
	rm -rf $(BUILD_DIR) $(BUILD_CONFIG)
