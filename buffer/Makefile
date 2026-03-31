CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
OBJ = buffer.o

all: test_buffer

buffer.o: buffer.c buffer.h
	$(CC) $(CFLAGS) -c buffer.c

test_buffer: test_buffer.c buffer.o
	$(CC) $(CFLAGS) -o test_buffer test_buffer.c buffer.o

clean:
	rm -f *.o test_buffer

