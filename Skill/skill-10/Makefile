CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = builtin_env_manager
SRC = builtin_env_manager.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o

test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test
