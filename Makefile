CC=gcc
CFLAGS=-g -Wall

mipd:
	$(CC) $(CFLAGS) main.c common.c lower/arp/cache.c upper/upper.c lower/mip/mip.c lower/arp/arp.c lower/mip/queue.c lower/util.c -o mipd

all: mipd

clean:
	rm -f mipd
