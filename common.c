#include <ifaddrs.h>
#include "common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <time.h>

int debug_flag = 0; // Global variable that represents if the deamon runs in debug-mode.

/**
 * Prints a debug message if the program is running in debug mode.
 * Accepts a format string and a variable number of arguments. Can therefore be used as printf.
 * Also prints a timestamp.
 * Uses the global variable debug_flag to check if debug-mode is active.
 * 
 * @param format The message to print, accepts format string.
 * 
 * @return void
 */
void global_debug(const char *format, ...) {
    if (debug_flag) {
        // Get the current time
        time_t now;
        struct tm newtime;
        char buf[80];

        time(&now);
        newtime = *localtime(&now);

        // Format time, "YYYY-MM-DD HH:MM:SS"
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &newtime);

        // Prints [TIMESTAMP][DEBUG]
        printf("[%s][DEBUG] ", buf);

        // Prints the debug-message
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n");
    }
}

/**
 * This function initializes the interface data for the given MIP address.
 * All interfaces that has AF_PACKET as sa_family and is NOT loopback is included.
 * 
 * @param ifs Pointer to an ifs_data structure, which the function will update with information about network interfaces.
 * 
 * @param rsock A raw socket descriptor. This descriptor is stored in the ifs_data structure.
 *
 * @param local_mip_addr The MIP address for the local node.
 *
 * @return Integer A result code. 0 on success and -1 on failure (e.g., if getifaddrs() fails)
 *
 * Uses the global function global_debug to log debug information. 
 */
int init_ifs(struct ifs_data *ifs, u_int8_t local_mip_addr)
 {
    global_debug("Initializing interface data");
    struct ifaddrs *ifaces, *ifp;
    int i = 0;

    if (getifaddrs(&ifaces)) {
        perror("getifaddrs");
        return -1;
    }

    for (ifp = ifaces; ifp != NULL; ifp = ifp->ifa_next) {
        if (ifp->ifa_addr != NULL &&
            ifp->ifa_addr->sa_family == AF_PACKET &&
            strcmp("lo", ifp->ifa_name)) {
            global_debug("Found interface: %s", ifp->ifa_name);
            memcpy(&(ifs->addr[i++]),
                   (struct sockaddr_ll*)ifp->ifa_addr,
                   sizeof(struct sockaddr_ll));
        }
    }
    ifs->ifn = i;
    ifs->local_mip_addr = local_mip_addr;

    freeifaddrs(ifaces);

    return 0;
}

/**
 * Retrieves the array index for the interface with the specified interface index.
 * This is necesarry since the logic for sending MIP packets relies on the array index for identifying the correct interface.
 *
 * @param ifs An ifs_data structure, which holds information about network interfaces.
 *
 * @param sll_ifindex The interface index to search for.
 *
 * @return Integer If a matching interface is found then returns its index in the array of structures. 
 * If no match is found, -1 is returned.
 *
 * Note: A valid ifs_data structure may be created with init_ifs().
 *
 */
int get_if_index(struct ifs_data ifs, int sll_ifindex) {
    int i;
    for (i = 0; i < ifs.ifn; i++) {
        if (ifs.addr[i].sll_ifindex == sll_ifindex) {
            return i;
        }
    }
    return -1;
}

/**
 * Creates a unix socket and binds it to the provided path for communication 
 * with upper layers. The socket is set up to use SOCK_SEQPACKET.
 *
 * @param socket_upper String representing the file system path to bind the socket to.
 *
 * @return Integer The file descriptor for the newly created socket if successful, -1 if any step in the 
 * socket creation, binding, or listening process fails.
 *
 * Uses global function perror to print out error messages to the standard output. 
 *
 */
int prepare_usd(char* socket_upper)
 {
    struct sockaddr_un addr;
    int usd = -1, rc = -1;

    usd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (usd < 0) {
        perror("socket() failed");
        return rc;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_upper, sizeof(addr.sun_path) - 1);
    unlink(socket_upper);

    rc = bind(usd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if (rc < 0) {
        perror("bind() failed");
        close(usd);
        return rc;
    }

    rc = listen(usd, 1);
    if (rc < 0) {
        perror("listen() failed");
        close(usd);
        return rc;
    }

    return usd;
}

/**
 * Creates a raw socket using the AF_PACKET and SOCK_RAW parameters.
 *
 * @return Integer Returns the file descriptor for the newly created socket if successful, -1 if socket creation fails.
 *
 */
int prepare_rsd() {
    int rsd;

    rsd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_MIP));
    if (rsd < 0) {
        perror("socket() failed");
        return -1;
    }

    return rsd;
}