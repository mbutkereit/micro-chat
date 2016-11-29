#ifndef Client_h
#define Client_h

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <stdlib.h>

#define SUPPORTED_VERSION 1
#define USERNAME_REAL_SIZE 15
#define USERNAME_PSEUDO_SIZE 16
#define DEFAULT_HOPS 0
#define MAX_MESSAGE_SIZE 255
#define ONLY_THREE_BIT_MASK 0x03

// Types
enum types {
	LOG_IN_OUT_HEADER = 1, CONTROL_INFO_HEADER = 2, MESSAGE_HEADER = 3
};
// Flags
enum flags {
	NO_FLAGS = 0, SYN = 1, ACK = 2, FIN = 4, DUP = 8, GET = 16
};

// Status
enum status_liste {
	PENDING = 0, QUEUED = 1, REMOVE = 2,
};

typedef struct {
	struct sockaddr_in client_addr;
	int socketFD;
	int status;
	int ttl;

} connection_item;

typedef struct {
	uint8_t type;
	uint8_t flags;
	uint8_t version;
	uint8_t length;
} common_header;

typedef struct {
	char username[USERNAME_REAL_SIZE];
} login_out_header;

typedef struct {
	char source_username[USERNAME_PSEUDO_SIZE];
	char destination_username[USERNAME_PSEUDO_SIZE];
} message_info;

struct controll_info {
	char username[USERNAME_PSEUDO_SIZE];
	uint32_t hops;
};
typedef struct controll_info controll_info;

typedef struct {
	common_header common_header;
	login_out_header login_out_header;
} login_out;

#endif /* Client_h */

