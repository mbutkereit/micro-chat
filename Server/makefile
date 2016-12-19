VERSION = 3.02
CC = gcc
OBJ = server_list.o chat_list.o socket_work_queue.o main.o
CFLAGS  = -g -Wall
LDFLAGS = -lpthread 
BIN_APPL = socker_server
default: all

all: socket_server 
socket_server: $(OBJ)
	$(CC) $(CFLAGS) -o socket_server $(OBJ) $(LDFLAGS)

server_list.o: server_list.c
	$(CC) $(CFLAGS) -c server_list.c
	
chat_list.o: chat_list.c
	$(CC) $(CFLAGS) -c chat_list.c

socket_work_queue.o: socket_work_queue.c
	$(CC) $(CFLAGS) -c socket_work_queue.c

main.o: main.c
	$(CC) $(CFLAGS) -c main.c
clean:
	rm -rf $(BIN_APPL) $(OBJ) 
