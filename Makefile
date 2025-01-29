# Compiler (Auto-detect)
CC ?= clang

# Detect OS and Architecture
UNAME_S := $(shell uname -s)
ARCH := $(shell uname -m)
CFLAGS_COMMON = -Wall -Wextra -I$(INC_DIR)
ifeq ($(UNAME_S), Darwin)
    CFLAGS_COMMON += -target $(ARCH)-apple-macos11
endif

# CFLAGS
CFLAGS_DEBUG = $(CFLAGS_COMMON) -g -O0
CFLAGS_RELEASE = $(CFLAGS_COMMON) -O3

# Verbose mode (default off, enable with V=1)
V ?= 0
ifeq ($(V),1)
    VERBOSE := 
else
    VERBOSE := @
endif

# Directories
WORKSPACE = EtherRecorder
PROJECT_DIR = $(WORKSPACE)/Project/$(ARCH)
BUILD_DIR = $(PROJECT_DIR)
BIN_DIR = $(WORKSPACE)/$(ARCH)
DEBUG_DIR = $(BUILD_DIR)/Debug
RELEASE_DIR = $(BUILD_DIR)/Release
DEBUG_BIN = $(BIN_DIR)/Debug
RELEASE_BIN = $(BIN_DIR)/Release

# Source and Object Files
SRC_DIR = $(WORKSPACE)/src
INC_DIR = $(WORKSPACE)/inc
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS_DEBUG = $(patsubst $(SRC_DIR)/%.c, $(DEBUG_DIR)/%.o, $(SRCS))
OBJS_RELEASE = $(patsubst $(SRC_DIR)/%.c, $(RELEASE_DIR)/%.o, $(SRCS))

# Target Executables
TARGET_DEBUG = $(DEBUG_BIN)/EtherRecorder
TARGET_RELEASE = $(RELEASE_BIN)/EtherRecorder

# Default target
all: debug release

# Debug build
debug: $(TARGET_DEBUG)

$(TARGET_DEBUG): $(OBJS_DEBUG) | $(DEBUG_BIN)
	$(VERBOSE) $(CC) $(CFLAGS_DEBUG) -o $@ $^
	@echo "[BUILD SUCCESS] Debug executable created: $@"

$(DEBUG_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(VERBOSE) $(CC) $(CFLAGS_DEBUG) -MMD -MP -c -o $@ $<
	@echo "[BUILD SUCCESS] Compiled: $< -> $@"

# Release build
release: $(TARGET_RELEASE)

$(TARGET_RELEASE): $(OBJS_RELEASE) | $(RELEASE_BIN)
	$(VERBOSE) $(CC) $(CFLAGS_RELEASE) -o $@ $^
	@echo "[BUILD SUCCESS] Release executable created: $@"

$(RELEASE_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(VERBOSE) $(CC) $(CFLAGS_RELEASE) -MMD -MP -c -o $@ $<
	@echo "[BUILD SUCCESS] Compiled: $< -> $@"

# Include dependencies
-include $(OBJS_DEBUG:.o=.d) $(OBJS_RELEASE:.o=.d)

# Create Binary Directories
$(DEBUG_BIN) $(RELEASE_BIN):
	@mkdir -p $@

# Clean targets
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

clean_debug:
	rm -rf $(DEBUG_DIR) $(DEBUG_BIN)

clean_release:
	rm -rf $(RELEASE_DIR) $(RELEASE_BIN)

clean_all: clean_debug clean_release

# Run targets
run_debug: debug
	$(VERBOSE) $(TARGET_DEBUG)

run_release: release
	$(VERBOSE) $(TARGET_RELEASE)

# Install target
install: release
	$(VERBOSE) cp $(TARGET_RELEASE) /usr/local/bin/EtherRecorder
	@echo "[INSTALL SUCCESS] Installed: /usr/local/bin/EtherRecorder"

# Help target
help:
	@echo "Available targets:"
	@echo "  make debug       - Compile debug build"
	@echo "  make release     - Compile release build"
	@echo "  make clean       - Remove all build artifacts"
	@echo "  make run_debug   - Run debug build"
	@echo "  make run_release - Run release build"
	@echo "  make install     - Install release binary to /usr/local/bin"
	@echo "  make V=1 ...     - Enable verbose mode"

.PHONY: all debug release clean clean_debug clean_release clean_all run_debug run_release install help
