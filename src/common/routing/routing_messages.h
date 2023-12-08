#ifndef MESSAGES_H
#define MESSAGES_H

#include <sys/types.h>
#include "../../routingd/table/table.h"

typedef struct __attribute__((packed)){
    u_int8_t mip_addr;
    u_int8_t ttl;
    u_int8_t id1;
    u_int8_t id2;
    u_int8_t id3;
} message_header;

typedef struct __attribute__((packed)){
    message_header header;
    u_int8_t buffer[511];
} general_message;

typedef struct __attribute__((packed)){
    message_header header;
} hello_message;

typedef struct __attribute__((packed)){
    message_header header;
    u_int8_t fastest_routes[MAX_NODES];
} update_message;

typedef struct {
    message_header header;
    u_int8_t mip_look_up;
} request_message;

typedef struct {
    message_header header;
    u_int8_t next_hop_mip;
} response_message;

#endif // MESSAGES_H
