CC = gcc
CFLAGS = -Wall -g
LIBS = -lpthread

all: libmsocket.a

libmsocket.a: mysocket.o
	ar rcs libmsocket.a mysocket.o

mysocket.o: mysocket.c mysocket.h
	$(CC) $(CFLAGS) -c mysocket.c

clean:
	rm -f *.o *.a