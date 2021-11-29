CC=gcc
CFLAGS=-Wall -fsanitize=address,undefined
LDFLAGS=-fsanitize=address,undefined
all: task2
prog1: task2.c
	$(CC) $(CFLAGS) -o task2 task2.c
.PHONY: clean all
clean:
	rm task2