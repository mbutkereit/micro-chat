#ifndef server_h
#define server_h
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <unistd.h>

#include "client.h"
#include "chat_list.h"
#include "socket_work_queue.h"

#define DEFAULT_PORT 9012 // Default Port laut Dokumentation.
#define ALLOWED_PENDING_CONNECTIONS 7 // Random Number.
#define MAXIMUM_TIME_TO_LIVE 20 
#define LOG_IN_NOT_POSSIBLE -1
#define LOG_IN_SUCCESSFUL 0
#define MAX_LOG_IN_TRYS 3
#define WORKER_THREADS 2
#define LOCAL_IP "127.0.0.1"
#define SELECT_WARTEZEIT 10

// Liste der Prototypen.
void sendControllInfo(connection_item* item , uint8_t flags);

#endif /* server_h */

