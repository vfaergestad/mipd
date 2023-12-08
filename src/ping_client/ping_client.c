#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <time.h>
#include "../mipd/mipd_common.h"

#define TIMEOUT 1

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
    printf("Usage: %s [-h] <socket_lower> <mip_addr> <message> <ttl> \n", argv[0]);
    printf("  -h\t\tPrints this help message\n");
    printf("  <socket_lower>\tPathname of the UNIX socket used to interface with lower layers.\n");
    printf("  <mip_addr>\tThe MIP address of the destination host\n");
    printf("  <message>\tThe message to send\n");
    printf("  <ttl>\t\tThe TTL of the message. (optional)\n");
}

/**
 * Prints a basic usage message and exits the program.
 * @param argv The command line arguments
 * @return void
 */
void usage_and_exit(char *argv[]) {
    printf("Usage: %s [-h] <socket_lower> <mip_addr> <message> \n", argv[0]);
    exit(EXIT_FAILURE);
}

/**
 * Main function for the ping program using the MIP protocol.
 *
 * Accepts destination host and TTL (default 8 seconds) as arguments. The program
 * interfaces with relevant services over a Unix domain socket. It sets up Unix
 * socket for communication and sends initial message indicating the type of data the
 * higher layers are supposed to handle with the lower layers by sending a specific SDU type.
 * Constructs a ping message and sends to destination host using the Unix socket.
 * It waits for a response from the destination host for a specfied TTL or a timeout.
 *
 * On getting a response from the host, it computes the Round Trip Time (RTT), prints the
 * received PING message along with the RTT. If no response is received from the host
 * within the specified TTL or a timeout, it prints a timeout message.
 *
 * @param argc: Integer, number of arguments passed to the program from the terminal.
 * @param argv: Array of strings representing the arguments passed to the program.
 *
 * @return: Integer representing the exit status of the program. 0 indicating normal termination
 * and exits with EXIT_FAILURE status on failure in setting up Unix socket or sending messages.
 */
int main(int argc, char *argv[]) {

    int opt, hflag = 0;
    const char *socket_lower, *destination_host, *message;

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
    if (argc - optind != 3 && argc - optind != 4) {
        usage_and_exit(argv);
    }

    // Set the arguments
    socket_lower = argv[optind];
    destination_host = argv[optind + 1];
    message = argv[optind + 2];
    // Check if the TTL was given, if not set it to 8
    int ttl = 8;
    if (argc - optind == 4) {
        ttl = atoi(argv[optind + 3]);
    }

    // Create unix socket for interfacing with lower layers
    int usd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (usd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_lower, sizeof(addr.sun_path) - 1);

    // Connect to the UNIX socket of the lower layer
    if (connect(usd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // Send an initial message to MIP daemon to identify the SDU type handled
    u_int8_t sdu_type = 0x02;  // SDU type for the ping protocol
    if (send(usd, &sdu_type, sizeof(sdu_type), 0) == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }

    // Construct the ping message
    char ping_message[256];
    snprintf(ping_message, sizeof(ping_message), "PING:%s", message);

    // Create the packet
    struct unix_message packet;
    packet.mip_addr = atoi(destination_host);
    strncpy(packet.message, message, sizeof(packet.message));
    packet.ttl = ttl;

    // Send the ping message
    if (send(usd, &packet, sizeof(packet), 0) == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }

    // Wait for a response or timeout
    struct epoll_event ev, events[1];
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = usd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, usd, &ev) == -1) {
        perror("epoll_ctl: usd");
        exit(EXIT_FAILURE);
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int nfds = epoll_wait(epollfd, events, 1, TIMEOUT * 1000); // timeout in milliseconds

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    if (nfds == 0) {
        printf("timeout\n");
    } else {
        // Read the pong message
        char buffer[256];
        recv(usd, buffer, sizeof(buffer), 0);
        // Check if the received message the same as the sent message
        char const *recv_message = strstr(buffer, ":") + 1; // Skip the PONG: part of the string
        if (memcmp(recv_message, message, strlen(message)) != 0) {
            printf("Received pong message does not match sent message\n");
        }
        printf("Received: %s, RTT: %.4f seconds\n", buffer, elapsed);
    }

    // Close the UNIX socket
    close(usd);

    return 0;
}