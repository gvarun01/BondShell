# Compiler and Linker
CC = gcc
# CFLAGS for compilation
# -Wall: Enable all warnings
# -Wextra: Enable extra warnings
# -g: Add debugging information
# -std=c11: Use C11 standard
# -Iinclude: Add include directory to header search path
# -Iutils: Add utils directory to header search path
# -D_POSIX_C_SOURCE=200809L: Define POSIX source for functions like strdup, usleep, kill, etc.
CFLAGS_COMMON = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude -Iutils
CFLAGS_RELEASE = $(CFLAGS_COMMON) -O2
CFLAGS_DEBUG = $(CFLAGS_COMMON) -g -DDEBUG
# LDFLAGS for linking
# -lm: Link math library (if needed, e.g. for animations or complex calculations)
# Add other libraries if needed, e.g. -lpthread if using pthreads
LDFLAGS = -lm

# Project name
TARGET = bondshell
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
SRC_DIR_CORE = src
SRC_DIR_UTILS = utils

# Source files
# Find all .c files in src and utils directories
SRCS_CORE = $(wildcard $(SRC_DIR_CORE)/*.c)
SRCS_UTILS = $(wildcard $(SRC_DIR_UTILS)/*.c)
SRCS = $(SRCS_CORE) $(SRCS_UTILS) main.c

# Object files: place them in OBJ_DIR, preserving subdirectory structure if complex
# For simplicity here, all objects go into $(OBJ_DIR)
# Replace src/ and utils/ prefixes with obj/ and change .c to .o
OBJS_CORE = $(patsubst $(SRC_DIR_CORE)/%.c,$(OBJ_DIR)/%.o,$(SRCS_CORE))
OBJS_UTILS = $(patsubst $(SRC_DIR_UTILS)/%.c,$(OBJ_DIR)/%.o,$(SRCS_UTILS))
OBJS_MAIN = $(patsubst %.c,$(OBJ_DIR)/%.o,main.c)
OBJS = $(OBJS_CORE) $(OBJS_UTILS) $(OBJS_MAIN)

# Default CFLAGS (can be overridden by debug target)
CFLAGS = $(CFLAGS_RELEASE)

# Default target: build all
all: $(BUILD_DIR)/$(TARGET)

# Target to build the executable
$(BUILD_DIR)/$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Build complete: $(BUILD_DIR)/$(TARGET)"

# Rule to compile source files into object files
# $<: the first prerequisite (the .c file)
# $@: the target file (the .o file)
$(OBJ_DIR)/%.o: $(SRC_DIR_CORE)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR_UTILS)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/main.o: main.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c main.c -o $(OBJ_DIR)/main.o


# Debug build
debug: CFLAGS = $(CFLAGS_DEBUG) # Override CFLAGS for debug
debug: clean all
	@echo "Debug build complete."

# Target to run the shell
run: all
	@echo "Running $(BUILD_DIR)/$(TARGET)..."
	./$(BUILD_DIR)/$(TARGET)

# Target to clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(OBJ_DIR)
	rm -f $(BUILD_DIR)/$(TARGET)
	rm -f bondshell # Remove if it was built in root previously
	rm -f a.out     # Remove default gcc output
	@echo "Clean complete."

# Phony targets (targets that don't represent actual files)
.PHONY: all clean run debug
