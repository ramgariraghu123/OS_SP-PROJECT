CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = history_pipeline_struct
SRC = history_pipeline_struct.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o

test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test
