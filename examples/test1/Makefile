# Compiler and flags
CC = gcc
CFLAGS = -L../out -lwt

TARGET = test

# Default target
all: $(TARGET)

$(TARGET): main.c
	$(CC) main.c $(CFLAGS) -o $@ 

.PHONY: all

