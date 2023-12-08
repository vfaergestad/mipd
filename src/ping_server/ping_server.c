#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include "../mipd/mipd_common.h"

struct unix_message {
    u_int8_t mip_addr;
    u_int8_t ttl;
    char message[511];
};

/**
 * Prints the help message
 * @param argv: The command line arguments
 * @return void
 */
void print_help(char *argv[]) {
    printf("Usage: %s [-h] <socket_lower>\n", argv[0]);
    printf("  -h\t\tPrints this help message\n");
    printf("  <socket_lower>\tpathname of the socket that the MIP daemon uses to communicate with upper layers .\n");
}

/**
 * Prints a basic usage message and exits the program.
 * @param argv The command line arguments
 * @return void
 */
void usage_and_exit(char *argv[]) {
    printf("Usage: %s [-h] <socket_lower>\n", argv[0]);
    exit(EXIT_FAILURE);
}


/**
 * Main function for a simplified ping program using the MIP protocol.
 *
 * The function uses Unix domain sockets to connect to services and starts a loop to
 * receive and handle messages. Depending on the flags provided, the function either
 * sends a help message and exits or checks if the correct number of arguments were given.
 *
 * The function creates a Unix socket and connects to it.
 * An initial identification message is then sent to the MIP daemon.
 * An epoll instance is created to handle multiple file descriptors.
 *
 * The function then enters a loop where it waits for incoming events.
 * If an event happens on the Unix socket, a response message is read and a PONG reply is prepared and sent back.
 * If an error occurs during any of these steps, the function exits with a failure status.
 *
 * @param argc: Integer, number of arguments passed to the program from the terminal.
 * @param argv: Array of strings representing the arguments passed to the program.
 *
 * @return: Integer representing the exit status of the program. 0 indicating normal termination,
 * and exits with EXIT_FAILURE status on failure in setting up Unix socket, connecting to services,
 * sending messages, or handling epoll events.
 */
int main(int argc, char *argv[]) {
    int opt;
    int hflag = 0; // Stores if the help-flag is given.
    char const *socket_lower; // Stores the pathname of the socket that the MIP daemon uses to communicate with upper layers.

    while ((opt = getopt(argc, argv, "h")) != -1) {
        switch (opt) {
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
    if (argc - optind != 1) {
        usage_and_exit(argv);
    }

    // Set the arguments
    socket_lower = argv[optind];

    // Create UNIX socket
    int usd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (usd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Connect to the socket
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_lower, sizeof(addr.sun_path) - 1);

    if (connect(usd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // Send an initial message to MIP daemon to identify the SDU type handled
    u_int8_t sdu_type = 0x02;  // SDU type for the ping protocol
    if (send(usd, &sdu_type, sizeof(sdu_type), 0) == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }

    // Create epoll instance
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev, events[10];  // Up to 10 events to process at once
    ev.events = EPOLLIN;
    ev.data.fd = usd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, usd, &ev) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    // Main loop for receiving and handling messages
    while (1) {
        int nfds = epoll_wait(epollfd, events, 10, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == usd) {
                // Received something on the Unix socket
                struct unix_message received_packet;
                ssize_t n = recv(usd, &received_packet, sizeof(received_packet), 0);
                if (n > 0) {

                    printf("Received from host %c: %s\n", received_packet.mip_addr, received_packet.message);

                    // Prepare a PONG response
                    struct unix_message response_packet;
                    response_packet.mip_addr = received_packet.mip_addr;
                    response_packet.ttl = 15; // Default TTL
                    snprintf(response_packet.message, sizeof(response_packet.message), "PONG:%s", received_packet.message);

                    // Send the response back
                    if (send(usd, &response_packet, sizeof(response_packet), 0) == -1) {
                        perror("send");
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
}