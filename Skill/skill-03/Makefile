CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = dynamic_buffer_history
SRC = dynamic_buffer_history.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o

test: $(TARGET)
	./$(TARGET) --demo

.PHONY: all clean test
