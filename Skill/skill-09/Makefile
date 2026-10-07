CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = cd_navigator
SRC = cd_navigator.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o

test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test
