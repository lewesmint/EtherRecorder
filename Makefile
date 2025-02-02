###############################################################################
# Combined Makefile for Windows (MSVC) and macOS (Clang)
# Preserves your Debug/Release structure, run targets, etc.
###############################################################################

PROJECT_NAME = EtherRecorder

# Detect if we're on Windows or not.
# OS=Windows_NT is typically set under Windows (including MSYS/MinGW).
# If not Windows, we'll check if it's macOS by checking uname.
ifeq ($(OS),Windows_NT)
	TARGET_OS = windows
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		TARGET_OS = macos
    else
		TARGET_OS = unknown
	endif
endif

###############################################################################
# Architecture Detection
###############################################################################
ifeq ($(TARGET_OS),windows)
	# Windows typically sets PROCESSOR_ARCHITECTURE. 
	# If it is x86, but PROCESSOR_IDENTIFIER contains "ARM", we assume ARM64 in emulation.
	ifeq ($(PROCESSOR_ARCHITECTURE),x86)
		ifneq (,$(findstring ARM,$(PROCESSOR_IDENTIFIER)))
			PLATFORM_ARCH = WinArm64
		else
			PLATFORM_ARCH = Win32
		endif
	else ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
		PLATFORM_ARCH = x64
    else ifeq ($(PROCESSOR_ARCHITECTURE),ARM64)
		PLATFORM_ARCH = WinArm64
    else
		PLATFORM_ARCH = WinUnknown
	endif
else ifeq ($(TARGET_OS),macos)
	# macOS: detect Apple Silicon vs Intel
	UNAME_M := $(shell uname -m)
	ifeq ($(UNAME_M),arm64)
		PLATFORM_ARCH = MacArm64
    else ifeq ($(UNAME_M),x86_64)
		PLATFORM_ARCH = x86_64Mac
    else
		PLATFORM_ARCH = MacUnknown
	endif
else
	PLATFORM_ARCH = $(TARGET_OS)_unknown
endif

###############################################################################
# Directories, Build Types, and File Extensions
###############################################################################
# Build type can be Debug or Release, default Debug. (Override with `make debug` or `make release`.)
BUILD_TYPE ?= Debug

# Top-level output directories
BUILD_DIR  = $(PROJECT_NAME)/$(PLATFORM_ARCH)
BIN_DIR    = $(PLATFORM_ARCH)

DEBUG_DIR      = $(BUILD_DIR)/Debug
RELEASE_DIR    = $(BUILD_DIR)/Release
DEBUG_BIN_DIR  = $(BIN_DIR)/Debug
RELEASE_BIN_DIR= $(BIN_DIR)/Release

# Source code / include directory
SRC_DIR = $(PROJECT_NAME)/src
INC_DIR = $(PROJECT_NAME)/inc

# Gather .c files
SRCS = $(wildcard $(SRC_DIR)/*.c)

###############################################################################
# Compiler and Flags (Windows vs macOS)
###############################################################################
ifeq ($(TARGET_OS),windows)
	############################################################################
	# Windows (MSVC)
	############################################################################
	CC          = cl
	# Executable extension
	EXE_EXT     = .exe

	# Common MSVC flags
	CFLAGS_COMMON = /nologo /TC /std:c17 /W3 /Zi /I $(INC_DIR) /D_CRT_SECURE_NO_WARNINGS

	# Debug vs Release
	CFLAGS_DEBUG   = $(CFLAGS_COMMON) /Od
	CFLAGS_RELEASE = $(CFLAGS_COMMON) /O2

	# Object file extension (.obj) 
	OBJ_EXT = .obj

	# Link command (simplified: add more flags if needed)
	LINK_CMD = $(CC)

	# Command to remove directories or files on Windows
	RM_RF = rmdir /S /Q
	RM_F  = del /Q

	# Use a shell-friendly echo
	ECHO  = echo

else ifeq ($(TARGET_OS),macos)
	############################################################################
	# macOS (Clang)
	############################################################################
	CC          = clang
	# No .exe extension on macOS
	EXE_EXT     =

	# Common clang flags
	CFLAGS_COMMON = -Wall -Wextra -pthread -I$(INC_DIR)

	# Debug vs Release
	CFLAGS_DEBUG   = $(CFLAGS_COMMON) -g -O0
	CFLAGS_RELEASE = $(CFLAGS_COMMON) -O3

	# Use .o for macOS
	OBJ_EXT = .o

	# Link command for clang
	LINK_CMD = $(CC)

	# Command to remove directories or files on macOS
	RM_RF = rm -rf
	RM_F  = rm -f

	ECHO  = echo

else
	############################################################################
	# Unknown or unsupported
	############################################################################
	CC          = clang
	EXE_EXT     =
	CFLAGS_DEBUG   = -Wall -Wextra -g -O0
	CFLAGS_RELEASE = -Wall -Wextra -O3
	OBJ_EXT        = .o
	LINK_CMD       = $(CC)
	RM_RF          = rm -rf
	RM_F           = rm -f
	ECHO           = echo
endif

###############################################################################
# Objects & Targets
###############################################################################
# Objects for Debug / Release
OBJS_DEBUG   = $(patsubst $(SRC_DIR)/%.c, $(DEBUG_DIR)/%$(OBJ_EXT),   $(SRCS))
OBJS_RELEASE = $(patsubst $(SRC_DIR)/%.c, $(RELEASE_DIR)/%$(OBJ_EXT), $(SRCS))

# Final executables (Debug / Release)
TARGET_DEBUG   = $(DEBUG_BIN_DIR)/$(PROJECT_NAME)$(EXE_EXT)
TARGET_RELEASE = $(RELEASE_BIN_DIR)/$(PROJECT_NAME)$(EXE_EXT)

###############################################################################
# Verbose / Quiet Mode
###############################################################################
# V=1 => Print commands, V=0 => Quiet
V ?= 0
ifeq ($(V),1)
	VERBOSE :=
else
	VERBOSE := @
endif

###############################################################################
# Default: build both debug and release
###############################################################################
all: debug release

###############################################################################
# Debug Build
###############################################################################
debug: $(TARGET_DEBUG)

# Linking debug executable
$(TARGET_DEBUG): $(OBJS_DEBUG) | $(DEBUG_BIN_DIR)
	@$(ECHO) "[LINK] Debug -> $@"
	$(VERBOSE) $(LINK_CMD) $(CFLAGS_DEBUG) -o $@ $^
	@$(ECHO) "[BUILD SUCCESS] Debug executable created: $@"

# Compiling debug .c -> .obj or .o
$(DEBUG_DIR)/%$(OBJ_EXT): $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@mkdir -p $(DEBUG_BIN_DIR)
	@$(ECHO) "[CC] Debug compile: $<"
	$(VERBOSE) $(CC) $(CFLAGS_DEBUG) -c $< -o $@

# Create debug bin directory
$(DEBUG_BIN_DIR):
	@mkdir -p $@

###############################################################################
# Release Build
###############################################################################
release: $(TARGET_RELEASE)

# Linking release executable
$(TARGET_RELEASE): $(OBJS_RELEASE) | $(RELEASE_BIN_DIR)
	@$(ECHO) "[LINK] Release -> $@"
	$(VERBOSE) $(LINK_CMD) $(CFLAGS_RELEASE) -o $@ $^
	@$(ECHO) "[BUILD SUCCESS] Release executable created: $@"

# Compiling release .c -> .obj or .o
$(RELEASE_DIR)/%$(OBJ_EXT): $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@mkdir -p $(RELEASE_BIN_DIR)
	@$(ECHO) "[CC] Release compile: $<"
	$(VERBOSE) $(CC) $(CFLAGS_RELEASE) -c $< -o $@

# Create release bin directory
$(RELEASE_BIN_DIR):
	@mkdir -p $@

###############################################################################
# Run Targets (macOS uses ./TARGET, Windows uses .exe)
###############################################################################
run_debug: debug
ifeq ($(TARGET_OS),windows)
	@$(ECHO) "Running debug: $(TARGET_DEBUG)"
	$(VERBOSE) $(TARGET_DEBUG)
else ifeq ($(TARGET_OS),macos)
	@$(ECHO) "Running debug: ./$(TARGET_DEBUG)"
	$(VERBOSE) ./$(TARGET_DEBUG)
else
	@$(ECHO) "run_debug not supported on this platform"
endif

run_release: release
ifeq ($(TARGET_OS),windows)
	@$(ECHO) "Running release: $(TARGET_RELEASE)"
	$(VERBOSE) $(TARGET_RELEASE)
else ifeq ($(TARGET_OS),macos)
	@$(ECHO) "Running release: ./$(TARGET_RELEASE)"
	$(VERBOSE) ./$(TARGET_RELEASE)
else
	@$(ECHO) "run_release not supported on this platform"
endif

###############################################################################
# Install Target (macOS only example)
###############################################################################
install: release
ifeq ($(TARGET_OS),macos)
	$(VERBOSE) cp $(TARGET_RELEASE) /usr/local/bin/$(PROJECT_NAME)
	@$(ECHO) "[INSTALL SUCCESS] Installed: /usr/local/bin/$(PROJECT_NAME)"
else
	@$(ECHO) "Install not supported on Windows"
endif

###############################################################################
# Cleaning Targets
###############################################################################
clean:
	@$(ECHO) "Cleaning project..."
	# Remove debug artifacts
	-$(RM_RF) "$(DEBUG_DIR)"
	-$(RM_RF) "$(DEBUG_BIN_DIR)"
	# Remove release artifacts
	-$(RM_RF) "$(RELEASE_DIR)"
	-$(RM_RF) "$(RELEASE_BIN_DIR)"
	@$(ECHO) "Clean complete."

clean_debug:
	@$(ECHO) "Cleaning debug build..."
	-$(RM_RF) "$(DEBUG_DIR)"
	-$(RM_RF) "$(DEBUG_BIN_DIR)"
	@$(ECHO) "Clean debug complete."

clean_release:
	@$(ECHO) "Cleaning release build..."
	-$(RM_RF) "$(RELEASE_DIR)"
	-$(RM_RF) "$(RELEASE_BIN_DIR)"
	@$(ECHO) "Clean release complete."

clean_all: clean_debug clean_release

###############################################################################
# Help
###############################################################################
help:
	@$(ECHO) "Available targets:"
	@$(ECHO) "  make debug        - Compile Debug build"
	@$(ECHO) "  make release      - Compile Release build"
	@$(ECHO) "  make clean        - Remove all build artifacts"
	@$(ECHO) "  make run_debug    - Run debug build (macOS/Windows only)"
	@$(ECHO) "  make run_release  - Run release build (macOS/Windows only)"
	@$(ECHO) "  make install      - (macOS) Install release binary to /usr/local/bin"
	@$(ECHO) "  make V=1         - Enable verbose mode"
	@$(ECHO) "  make all         - Build both Debug and Release"

.PHONY: all debug release clean clean_debug clean_release clean_all run_debug run_release install help