CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = multi_pipe_exec
SRC = multi_pipe_exec.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o

test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test
