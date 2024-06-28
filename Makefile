# Compiler and compiler flags
CC = gcc
CFLAGS = -Wall -g

# Define the target executable name
TARGET = myserver

# Define source and object files
SRCS = myserver.c
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Link the object files into the final executable
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $

# Delete object files and executable to start fresh
clean:
	rm -f $(OBJS) $(TARGET)

# Suffix rule to make handling dependencies easier
.PHONY: all clean
