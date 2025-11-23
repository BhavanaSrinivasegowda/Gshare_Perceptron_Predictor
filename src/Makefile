#========================================================
# Makefile for Branch Predictor Project
#========================================================

CC = gcc
CFLAGS = -Wall -std=c11 -O2
TARGET = predictor_app

SRC = main.c predictor.c
OBJ = $(SRC:.c=.o)

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile .c -> .o
%.o: %.c predictor.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJ) $(TARGET)

