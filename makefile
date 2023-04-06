# Compiler options
CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -std=c99

# Target program
TARGET = ciminx.exe

# Source files
SRCS = ciminx.c

# Object files
OBJS = $(SRCS:.c=.o)

# Paths to libraries and includes
LIBS = -lm
INCLUDES = -I./jsmn

# Default target
all: $(TARGET)

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Link target program
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) $^ -o $@

# Run target program with parameters
run:
	./$(TARGET) -i ./jsmn/testcases/crash.json -m "AddressSanitizer: heap-buffer-overflow" -o reduced ./jsmn/jsondump

# Clean up object files and target program
clean:
	rm -f $(OBJS) $(TARGET)
