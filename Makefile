CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
TARGET = line_edito

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

clean:
	rm -f $(TARGET) demo.txt test.txt notes.txt

.PHONY: all clean
