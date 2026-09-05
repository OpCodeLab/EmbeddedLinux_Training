CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lpthread -lrt

server: main.c cJSON.c
	$(CC) $(CFLAGS) -o server main.c cJSON.c $(LDFLAGS)

clean:
	rm -f server
