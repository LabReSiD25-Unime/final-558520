CC = gcc
CFLAGS = -Wall

all: server

server: server.c client.c connection.c
	$(CC) $(CFLAGS) -o server server.c client.c connection.c -lrt

clean:
	rm -f server