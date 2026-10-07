CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = interactive_loop
SRC = interactive_loop.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o

test: $(TARGET)
	./$(TARGET) --demo

.PHONY: all clean test
