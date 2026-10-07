CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = process_hierarchy
SRC = process_hierarchy.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o

test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test
