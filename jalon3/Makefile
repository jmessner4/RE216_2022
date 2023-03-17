CFLAGS=-Wall
#LDFLAGS=-lpthread
all: client server
	gcc $(CFLAGS) -o client client.c
	gcc $(CFLAGS) -o server server.c
c: client
	gcc $(CFLAGS) -o client client.c
s: server
	gcc $(CFLAGS) -o server server.c
clean:
	rm -f client server