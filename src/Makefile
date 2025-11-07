# =====================================================
#  Makefile for Gshare Branch Predictor
# =====================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2

# Executable name
TARGET = predictor

# Object files
OBJS = predictor.o main.o

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lm

# Compile source files
predictor.o: predictor.c predictor.h
	$(CC) $(CFLAGS) -c predictor.c

main.o: main.c predictor.h
	$(CC) $(CFLAGS) -c main.c

# Run simulation with an input file
run: $(TARGET)
	./$(TARGET) input.txt

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

