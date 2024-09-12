# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Source and object directories
SRC_DIR = src
OBJ_DIR = obj

# Find all .c files in src directory, including subdirectories
SRCS = $(shell find $(SRC_DIR) -name '*.c')

# Generate object files with the same directory structure in obj
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Output executable name
TARGET = program

# Static library file
STATIC_LIB = libwt.a

# Default target
# all: $(TARGET)
all: $(STATIC_LIB)

$(STATIC_LIB) : $(OBJS)
	# mkdir out
	ar rcs out/$@ $(OBJS)

# # Rule for linking the final executable
# $(TARGET): $(OBJS)
# 	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Rule to compile each .c file into an .o file (create nested directories if needed)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)  # Ensure the directory exists
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up object files and the executable
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Phony targets
.PHONY: all clean
