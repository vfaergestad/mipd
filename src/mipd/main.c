#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "mipd_common.h"
#include "lower/arp/cache.h"
#include "upper/upper.h"
#include "lower/mip/queues/arp_queue.h"
#include "lower/lower.h"
#include "lower/mip/queues/route_queue.h"

/**
 * Prints the help message
 * @param argv: The command line arguments
 * @return void
 */
void print_help(char *argv[]) {
    printf("Usage: %s [-h] [-d] <socket_upper> <MIP address>\n", argv[0]);
    printf("  -h\t\tPrints this help message\n");
    printf("  -d\t\tRuns the program in debug mode\n");
    printf("  <socket_upper>\tPathname of the UNIX socket used to interface with upper layers.\n");
    printf("  <MIP address>\tThe MIP address to assign to this host\n");
}

/**
 * Prints a basic usage message and exits the program.
 * @param argv The command line arguments
 * @return void
 */
void usage_and_exit(char *argv[]) {
    printf("Usage: %s [-h] [-d] <socket_upper> <MIP address>\n", argv[0]);
    exit(EXIT_FAILURE);
}

/**
 * Main function for the daemon process for mip communication.
 *
 *
 * @param argc: Integer, number of arguments passed to the program from the terminal.
 * @param argv: Array of strings representing the arguments passed to the program.
 *
 * @return: Integer, it returns -1 upon any error that may interrupt the program's execution, such as failure
 *          in creating epoll instances or failures in setting up raw and unix socket descriptors (usd). If no
 *          errors encountered, it runs indefinitely (while(1) in main-loop), hence doesn't return anything.
 */
int main(int argc,char *argv[]) {


    int opt;            // Stores the given command-options.
    int hflag = 0;      // Stores if the help-flag is given.
    int mip_addr = 0;   // The given MIP-address of this node. Initialized as 0.
    char *socket_upper; // String for storing the name of the unix-socket used to communicate with the upper layers.

    // Loop through the given options to set the appropriate flags.
    while ((opt = getopt(argc, argv, "dh")) != -1) {
        switch (opt) {
            case 'd':
                // Running in debug mode
                debug_flag = 1; // This variable is declared in common.h.
                global_debug("Running in debug mode");
                break;
            case 'h':
                // Help flag given
                hflag = 1;
                break;
            default:
                usage_and_exit(argv);
        }
    }

    // Check if the help flag was given, if so print help message and exit
    if (hflag) {
        print_help(argv);
        exit(EXIT_SUCCESS);
    }

    // Check if the correct number of arguments were given
    if (argc - optind != 2) {
        usage_and_exit(argv);
    } else {
        socket_upper = argv[optind];
        mip_addr = atoi(argv[optind + 1]);
        global_debug("Socket upper: %s", socket_upper);
        global_debug("MIP address: %d", mip_addr);
    }

    // Initialize the arp_cache, route_queue and arp_queue, needs to be done before receiving any events.
    arp_cache_init();
    init_arp_queue();
    init_route_queue();

    struct fds fds; // Struct that stores the different socket file-descriptors used by the daemon. Defined in common.h.

    // Create raw socket for sending and receiving MIP packets
    int rsd = prepare_rsd();
    if (rsd < 0) {
        perror("prepare_rsd() failed");
        return -1;
    }

    // Create unix socket for interfacing with upper layers
    int usd = prepare_usd(socket_upper);
    if (usd < 0) {
        perror("prepare_usd() failed");
        return -1;
    }

    fds.rsd = rsd;
    fds.usd = usd;
    fds.num_accepted_usds = 0;

    // Initialize interface data
    struct ifs_data ifs_data;
    if (init_ifs(&ifs_data, mip_addr) < 0) {
        perror("init_ifs() failed");
        return -1;
    }

    // Create epoll instance
    struct epoll_event ev_raw, ev_usd, events[MAX_EVENTS];
    int epollfd;

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        return -1;
    }

    // Add raw-socket to the epoll-table
    ev_raw.events = EPOLLIN;
    ev_raw.data.fd = rsd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, rsd, &ev_raw) == -1) {
        perror("epoll_ctl: rsd");
        return -1;
    }

    // Add unix-socket to the epoll-table
    ev_usd.events = EPOLLIN;
    ev_usd.data.fd = usd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, usd, &ev_usd) == -1) {
        perror("epoll_ctl: usd");
        return -1;
    }

    // Main loop. Waits indefinitely for events on the sockets and handles them when they arise.
    int num_events;
    global_debug("Entering main loop");
    while (1) {
        num_events = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
            perror("epoll_wait");
            return -1;
        }

        for (int i = 0; i < num_events; i++) {


            // ----------------- Unix Socket -----------------
            if (events[i].data.fd == usd) {
                // Someone is trying to connect() to the unix socket
                //global_debug("Received event from unix socket");

                // Accept the connection
                //global_debug("Accepting connection");
                int accept_usd = handle_usd_request(&fds);
                if (accept_usd < 0) {
                    global_debug("handle_usd_request() failed");
                    close(usd);
                    continue;
                }

                // Add the new socket to the epoll instance
                //global_debug("Adding new socket to epoll instance");
                struct epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.fd = accept_usd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, accept_usd, &ev) == -1) {
                    perror("epoll_ctl: accept_usd");
                    close(accept_usd);
                    continue;
                }

            // ----------------- Raw Socket -----------------
            } else if (events[i].data.fd == rsd) {
                // Raw socket event
                global_debug("Received event from raw socket");

                // Receive the packet
                int err = handle_rsd_event(fds, ifs_data);
                if (err < 0) {
                    global_debug("handle_rsd_event() failed");
                    close(rsd);
                    continue;
                }
            }


            // ----------------- Accepted Unix Sockets -----------------
            else {
                // Loop through the accepted unix sockets to find the one that received the event
                int match = 0;
                for (int j = 0; j < fds.num_accepted_usds; j++)
                    if (events[i].data.fd == fds.accepted_usds[j].usd) {
                        match = 1;
                        //global_debug("Received event from accepted unix socket");
                        //global_debug("Type: %d", fds.accepted_usds[j].type);
                        int err = handle_usd_event(fds, ifs_data, fds.accepted_usds[j]);
                        if (err < 0) {
                            //if (err == -1) {
                            //    perror("handle_usd_event() failed. Closing socket.");
                            // } else
                            if (err == -2) {
                                global_debug("EOF received. Closing socket.");
                            }
                            close(fds.accepted_usds[j].usd);
                            // Remove usd from accepted_usds
                            for (int k = j; k < fds.num_accepted_usds - 1; k++) {
                                fds.accepted_usds[k] = fds.accepted_usds[k + 1];
                            }
                            fds.num_accepted_usds--;
                            continue;
                        }
                    }

                if (!match) {
                    // Unknown event
                    global_debug("Received event from unknown file descriptor");
                }
            }
        }
    }
}
