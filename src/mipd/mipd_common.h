#include <stdarg.h>
#include <stdio.h>
#include <linux/if_packet.h>
#include <sys/types.h>

#ifndef COMMON_H
#define COMMON_H

extern int debug_flag; // Global variable that represents if the daemon runs in debug-mode.

#define MAX_IFS                 10      // Maximum number of interfaces that can be stored in an ifs_data structure.
#define ETH_P_MIP               0x88B5  // The ethernet-type used my MIP packets.
#define MAX_EVENTS              2       // Maximum amount of events that can wait in the epoll-loop at a time.
#define MAX_ACCEPTED_USDS       10      // Maximum amount of accepted unix-sockets that can be stored in the fds structure.


/**
 * Structure for storing data about the network interfaces on the local node.
 * Holds a maximum of MAX_IFS number of interface addresses.
 * Initialized by init_ifs().
*/
struct ifs_data{
    struct sockaddr_ll addr[MAX_IFS];   // MAC-addresses of the interfaces.
    int ifn;                            // The number of interfaces stored in the structure.
    u_int8_t local_mip_addr;            // The MIP address of the local node.
};

struct accepted_usd {
    int usd;
    u_int8_t type;
};

/**
 * Structure for storing the different socket file-descriptors used by the daemon.
*/
struct fds {
    int rsd;        // The raw-socket file-descriptor used to communicate with the lower layers.
    int usd;        // The unix-socket file-descriptor used to receive connections from the upper layers.
    struct accepted_usd accepted_usds[MAX_ACCEPTED_USDS]; // The unix-socket file-descriptors created when accepting a connection from the upper layers.
    int num_accepted_usds; // The number of accepted unix-socket file-descriptors.
    int routing_usd; // The unix-socket file-descriptor that is used for routing.
    int ping_usd; // The unix-socket file-descriptor that is relevant for the current event.
};


void global_debug(const char *format, ...);

int init_ifs(struct ifs_data *ifs, u_int8_t local_mip_addr);

int get_if_index(struct ifs_data ifs, int sll_ifindex);

int prepare_usd(const char* socket_upper);

int prepare_rsd();

#endif //COMMON_H
