CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = path_sync_exec
SRC = path_sync_exec.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o

test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test
