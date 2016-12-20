#ifndef server_list_h
#define server_list_h

#include <stdio.h>
#include <stdlib.h>
#include "client.h"
// Only a copy of Freebsd sys/queue.h
#include "queue.h"
#include "chat_list.h"

 struct server_list{
	connection_item* connection_item;
	LIST_ENTRY(server_list) entries_server;
};


typedef struct server_list server_list;

void init_server_list();
void add_to_server_list(connection_item* item);
void check_event_list(fd_set* test);
void remove_from_Serverlist_by_Socket(int SocketFD, int notify);

#endif /* chat_list_h */
