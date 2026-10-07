CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = var_dispatch_shell
SRC = var_dispatch_shell.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o

test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test
