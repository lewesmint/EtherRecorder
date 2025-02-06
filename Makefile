###############################################################################
# MakeWin.mak — MSVC C Makefile with Architecture Detection and Minimal Output
###############################################################################

# Project name
PROJECT_NAME = EtherRecorder

# Determine architecture from environment
ifeq ($(PROCESSOR_ARCHITECTURE),x86)
    # Possibly on real x86 or ARM64 in x86 emulation.
    ifneq (,$(findstring ARM,$(PROCESSOR_IDENTIFIER)))
        ARCH = ARM64
    else
        ARCH = Win32
    endif
else ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
    ARCH = x64
else ifeq ($(PROCESSOR_ARCHITECTURE),ARM64)
    ARCH = ARM64
else
    ARCH = unknown
endif

# Build type can be set by "make debug" or "make release"; default is Debug
BUILD_TYPE ?= Debug

# Output folder for the final .exe, e.g. WinArm64\Debug
OUTDIR = $(ARCH)\$(BUILD_TYPE)
# Folder for intermediate object files, e.g. EtherRecorder\WinArm64\Debug
OBJDIR = $(PROJECT_NAME)\$(ARCH)\$(BUILD_TYPE)

# Source code and include path
SRCDIR = $(PROJECT_NAME)\src
INCDIR = $(PROJECT_NAME)\inc

# MSVC compiler and flags (no /machine:... flags!)
CC = cl

# Set runtime library flags based on build type
ifeq ($(BUILD_TYPE),Debug)
    RUNTIME_LIB = /MTd
else
    RUNTIME_LIB = /MT
endif

# Linked libraries
LIBS = shlwapi.lib ws2_32.lib bcrypt.lib 

# Compiler flags (only for compilation, not linking)
CFLAGS = /TC /std:c17 /W3 /Zi /I $(INCDIR) /D_CRT_SECURE_NO_WARNINGS $(RUNTIME_LIB)

# Linker flags (used only during linking, not compilation)
LDFLAGS = $(RUNTIME_LIB) /DEBUG /Zi /Fd:$(OUTDIR)/EtherRecorder.pdb
LINKFLAGS = /link /OUT:$(OUTDIR)/$(PROJECT_NAME).exe $(LIBS)

# Find all .c files in SRCDIR
SOURCES = $(wildcard $(SRCDIR)/*.c)
# Convert them to corresponding object files in OBJDIR
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.obj)

# Verbose toggle (1 = show "Compiling..." and "Linking...", 0 = quiet)
VERBOSE ?= 1

###############################################################################
# Default target: build everything
###############################################################################
all: create_dirs $(OUTDIR)\$(PROJECT_NAME).exe

###############################################################################
# Create output and object directories if they don't exist
###############################################################################
create_dirs:
	@if not exist "$(OUTDIR)" mkdir "$(OUTDIR)"
	@if not exist "$(OBJDIR)" mkdir "$(OBJDIR)"

###############################################################################
# Linking: produce the final .exe
###############################################################################
$(OUTDIR)\$(PROJECT_NAME).exe: $(OBJECTS)
ifeq ($(VERBOSE),1)
	@echo Linking $(PROJECT_NAME).exe into "$(OUTDIR)" ...
endif
	$(CC) $(OBJECTS) $(LDFLAGS) $(LINKFLAGS)
ifeq ($(VERBOSE),1)
	@echo Finished linking $(PROJECT_NAME).exe
endif

###############################################################################
# Compilation: each .c -> .obj
###############################################################################
$(OBJDIR)/%.obj: $(SRCDIR)/%.c | create_dirs
ifeq ($(VERBOSE),1)
	@echo Compiling $<
endif
	$(CC) $(CFLAGS) /c $< /Fo$@

###############################################################################
# Clean: remove final .exe and build directories
###############################################################################
clean:
	@if exist "$(OUTDIR)\$(PROJECT_NAME).exe" del /Q "$(OUTDIR)\$(PROJECT_NAME).exe"
	@if exist "$(OBJDIR)" rmdir /S /Q "$(OBJDIR)"
	@if exist "$(OUTDIR)" rmdir /S /Q "$(OUTDIR)"
	@echo Finished cleaning build files.

###############################################################################
# Debug and Release shortcuts
###############################################################################
debug: BUILD_TYPE = Debug
debug: all

release: BUILD_TYPE = Release
release: all

# Mark these as phony so make won't look for real files named "all", "clean", etc.
.PHONY: all create_dirs clean debug release