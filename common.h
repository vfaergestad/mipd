#include <stdarg.h>
#include <stdio.h>
#include <linux/if_packet.h>

#ifndef COMMON_H
#define COMMON_H

extern int debug_flag; // Global variable that represents if the deamon runs in debug-mode.

#define MAX_IFS                 10      // Maximum number of interfaces that can be stored in an ifs_data structure.
#define ETH_P_MIP               0x88B5  // The ethernet-type used my MIP packets.
#define MAX_EVENTS              2       // Maximum amount of events that can wait in the epoll-loop at a time.


/**
 * Structure for storing data about the network interfaces on the local node.
 * Holds a maximum of MAX_IFS number of interface addresses.
 * Initilized by init_ifs().
*/
struct ifs_data{
    struct sockaddr_ll addr[MAX_IFS];   // MAC-addresses of the interfaces.
    int ifn;                            // The number of interfaces stored in the structure.
    u_int8_t local_mip_addr;            // The MIP address of the local node.
};

/**
 * Structure for storing the different socket file-descriptors used by the deamon.
*/
struct fds {
    int rsd;        // The raw-socket file-descriptor used to communicate with the lower layers.
    int usd;        // The unix-socket file-descriptor used to receive connections from the upper layers.
    int usd_ping;   // The unix-socket file-descriptor created when accepting a connection from the upper layers.
};

/**
 * Structure for representing a ping-packet that is sent up to the upper layers.
*/
struct ping_packet {
    u_int8_t mip_addr;  // Either the sender or destination MIP address when passing up/down, respectively.
    char message[255];  // The message payload.
};


void global_debug(const char *format, ...);

int init_ifs(struct ifs_data *ifs, u_int8_t local_mip_addr);

int get_if_index(struct ifs_data ifs, int sll_ifindex);

int prepare_usd(char* socket_upper);

int prepare_rsd();

#endif //COMMON_H
