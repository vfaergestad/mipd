#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include "table/table.h"
#include "../common/routing/routing_messages.h"
#include "time.h"
#include "hello/hello.h"
#include "handle_messages.h"
#include "routing_common.h"
#include "hello/checkin.h"
#include "update/update.h"

/**
 * Prints the help message
 * @param argv: The command line arguments
 * @return void
 */
void print_help(char *argv[]) {
    printf("Usage: %s [-h] [-d] <socket_routing>\n", argv[0]);
    printf("  -h\t\tPrints this help message\n");
    printf("  -d\t\tRuns the program in debug mode\n");
    printf("  <socket_routing>\tpathname of the socket that the routing daemon uses to communicate.\n");
}

/**
* Prints a basic usage message and exits the program.
* @param argv The command line arguments
* @return void
*/
void usage_and_exit(char *argv[]) {
    printf("Usage: %s [-h] [-d] <socket_routing>\n", argv[0]);
    exit(EXIT_FAILURE);
}

/**
 * The main function for a routing daemon using the MIP protocol.
 *
 * This function parses command line arguments and determines whether to run in debug mode or not.
 * If '-h' flag is seen, a help message is printed and the program exits.
 * The function also initializes the routing table and its socket routing for communication.
 * The socket is created and connected to the appropriate network interface.
 * Initial message is then sent to the routing daemon to identify the sdu type handled.
 *
 * An epoll instance is created to handle multiple file descriptors.
 * The function then enters a main loop where it waits for incoming events if they happen on the socket,
 * handles the routing protocol messages and sends HELLO messages periodically.
 * It also checks if it's time to send a HELLO and UPDATE message and checks whether any neighbours have timed out.
 *
 * @param argc: Integer, number of arguments passed to the program from the terminal.
 * @param argv: Array of strings representing the arguments passed to the program.
 *
 * @return: Integer representing the exit status of the program. 0 indicating normal termination,
 * and exits with EXIT_FAILURE status on failure in setting up the socket, connecting to services,
 * sending messages, handling epoll events or creating the thread for sending periodic messages.
 */
int main(int argc, char *argv[]) {
    int opt, hflag = 0;
    char *socket_routing;

    while ((opt = getopt(argc, argv, "dh")) != -1) {
        switch (opt) {
            case 'd':
                // Running in debug mode
                debug_flag = 1; // This variable is declared in common.h.
                global_debug("Running in debug mode");
                break;
            case 'h':
                hflag = 1;
                break;
            default:
                usage_and_exit(argv);
        }
    }

    if (hflag) {
        print_help(argv);
        exit(EXIT_SUCCESS);
    }

    if (argc - optind != 1) {
        usage_and_exit(argv);
    }

    init_routing_table();

    socket_routing = argv[optind];

    // Create and set up the UNIX socket
    int usd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (usd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    global_debug("Socket routing: %s\n", socket_routing);
    // Connect to the socket
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_routing, sizeof(addr.sun_path) - 1);

    global_debug("Connecting to socket\n");
    if (connect(usd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // Send an initial message to MIP daemon to identify the SDU type handled
    global_debug("Sending SDU type\n");
    u_int8_t sdu_type = 0x04;  // SDU type for the routing protocol
    if (send(usd, &sdu_type, sizeof(sdu_type), 0) == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }

    // Create thread for sending periodic HELLO messages
    //pthread_t hello_thread;
    //if (pthread_create(&hello_thread, NULL, send_periodic_hello_message, &usd) != 0) {
    //    perror("pthread_create failed");
    //    exit(EXIT_FAILURE);
    //}

    // Create and set up epoll
    global_debug("Setting up epoll\n");
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev, events[10];
    ev.events = EPOLLIN;
    ev.data.fd = usd;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, usd, &ev) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    // Set up the hello timer and related variables
    u_int8_t neighbours[MAX_NODES];
    init_checkins();
    time_t current_time;
    int hello_interval = 5;
    int hello_timeout = 10;
    u_int8_t any_timeout = 0;

    // Send initial HELLO message
    send_hello_message(usd);
    time_t last_hello_time = time(NULL);
    time_t last_timeout_check = time(NULL);


    // Main loop for handling routing messages
    global_debug("Entering main loop\n");
    while (1) {
        int nfds = epoll_wait(epollfd, events, 10, 100);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == usd) {
                // Handle routing protocol messages
                //global_debug("Received routing protocol message\n");
                general_message received_message;
                long rc = recv(usd, &received_message, sizeof(received_message), 0);
                if (rc <= 0) {
                    continue;
                }

                handle_message(usd, received_message);

            }
        }

        // Check if it is time to send a HELLO message
        current_time = time(NULL);
        if (current_time - last_hello_time >= hello_interval) {
            // Send HELLO message
            send_hello_message(usd);
            last_hello_time = current_time;
        }

        // Check if it is time to check if any neighbours have timed out
        current_time = time(NULL);
        if (current_time - last_timeout_check >= hello_timeout) {
            // Check if any neighbours have timed out
            last_timeout_check = current_time;
            global_debug("Checking for timed out neighbours\n");
            get_all_neighbours(neighbours);
            for (int i = 0; i < MAX_NODES; i++) {
                if (neighbours[i] != 0) {
                    global_debug("Checking neighbour %d\n", i);
                    if (has_node_checked_in(i) == 0) {
                        global_debug("Neighbour %d has not checked in\n", i);
                        // This neighbour has timed out
                        global_debug("Neighbour %d has timed out\n", i);
                        set_hop_unreachable(i);
                        print_routing_table();
                        any_timeout = 1;
                    } else {
                        // Reset the checkin
                        uncheckin_node(i);
                    }
                }
            }
            if (any_timeout) {
                // Send UPDATE messages to all neighbours
                send_update_messages(usd);
                any_timeout = 0;
            }
        }
    }
}
