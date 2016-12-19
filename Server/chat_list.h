#ifndef chat_list_h
#define chat_list_h

#include <stdio.h>
#include <stdlib.h>
#include "client.h"
// Only a copy of Freebsd sys/queue.h
#include "queue.h"
#include "server.h"
#include "socket_work_queue.h"
#include "server_list.h"

 struct controll_info_list{
	controll_info controll_info;
	connection_item* connection_item;
	LIST_ENTRY(controll_info_list)
	entries;
};


typedef struct controll_info_list controll_info_list;


// Liste der Prototypen.
void add_new_controll_info(char username[USERNAME_REAL_SIZE], uint16_t hops, connection_item* item);
int checkUserAlreadyExist(char username[USERNAME_REAL_SIZE]);
controll_info_list* findUserByName(char* username);
int checkUserConnectedWithMe(char* username);
void writeChatListToBuffer(FILE* socketStream);
int find_next_smaller_socket_id();
void init_chat_list();
void checkEvent(fd_set*);
controll_info_list* findUserByName(char* username);
controll_info_list* _find_user_by_socket(int);
int merge_user_list(connection_item* item ,controll_info* user);
int remove_user_by_socket(int socketFD);
void notify_all_by_update();
void checkEventChatList(fd_set* test);

#endif /* chat_list_h */
