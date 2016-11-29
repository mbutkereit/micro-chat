VERSION = 3.02
CC = gcc
OBJ = main.o
CFLAGS  = -g -Wall
LDFLAGS = -lpthread 
BIN_APPL = socker_client
default: all

all: socket_client
socket_client: $(OBJ)
	$(CC) $(CFLAGS) -o socket_client $(OBJ) $(LDFLAGS)
main.o: main.c
	$(CC) $(CFLAGS) -c main.c
clean:
	rm -rf $(BIN_APPL) $(OBJ) 
