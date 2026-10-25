CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -pthread
LDFLAGS = -lm

all: determinant

determinant: main.c
	$(CC) $(CFLAGS) -o determinant main.c $(LDFLAGS)

clean:
	rm -f determinant

.PHONY: all clean
