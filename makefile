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
run_jsmn:
	./$(TARGET) -i ../OperatingSystem/jsmn/testcases/crash.json -m "AddressSanitizer: heap-buffer-overflow" -o reduced ../OperatingSystem/jsmn/jsondump
# Run target program with parameters
run_libxml2:
	./$(TARGET) -i ../OperatingSystem/libxml2/testcases/crash.xml -m "AddressSanitizer: heap-buffer-overflow" -o reduced ../OperatingSystem/libxml2/xmllint
# Run target program with parameters
run_balance:
	./$(TARGET) -i ../OperatingSystem/balance/testcases/fail -m "AddressSanitizer: heap-buffer-overflow" -o reduced ../OperatingSystem/balance/balance

# Clean up object files and target program
clean:
	rm -f $(OBJS) $(TARGET)
