CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = escape_exec
SRC = escape_exec.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o

test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test
