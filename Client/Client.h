//
//  Client.h
//  SocketClient
//
//  Created by Marvin Butkereit on 19.11.16.
//  Copyright © 2016 Marvin Butkereit. All rights reserved.
//

#ifndef Client_h
#define Client_h

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define SUPPORTED_VERSION 1
#define USERNAME_SIZE 15

// Types
enum types {
    LOG_IN_OUT_HEADER = 1,
    CONTROL_INFO_HEADER = 2,
    MESSAGE_HEADER = 3
};
// Flags
enum flags {
    NO_FLAG = 0,
    SYN = 1,
    ACK = 2,
    FIN = 4,
    DUP = 8,
    GET = 16
};

typedef struct{
    uint8_t type;
    uint8_t flags;
    uint8_t version;
    uint8_t length;
}common_header;

typedef struct{
    char username[USERNAME_SIZE];
}login_out_header;

typedef struct{
    char username[USERNAME_SIZE];
    uint16_t hops;
} controll_info;

typedef struct{
    char source_username[USERNAME_SIZE];
    char destination_username[USERNAME_SIZE];
    char payload[255];
} message_info;


typedef struct{
    common_header common_header;
    login_out_header login_out_header;
}login_out;

#endif /* Client_h */
