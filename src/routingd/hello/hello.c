#include <sys/socket.h>
#include <stdio.h>
#include "../table/table.h"
#include "../../common/routing/routing_messages.h"
#include "../routing_common.h"
#include "../update/update.h"
#include "hello.h"
#include "checkin.h"

#define MIP_BROADCAST 255
#define BROADCAST_TTL 1

/**
 * Function to process 'HELLO' messages received in the MIP routing protocol.
 *
 * This function handles the response to incoming 'HELLO' messages, managing updates
 * to routing tables and communicating changes to other nodes.
 *
 * @param usd: The file descriptor of the unix socket used for routing communication.
 * @param message: The 'HELLO' message that was received.
 *
 */
void handle_hello_message(int usd, const hello_message message) {
    global_debug("Received HELLO message\n");
    uint8_t sender_mip = message.header.mip_addr;
    // Check in the neighbour
    checkin_node(sender_mip);
    // Neighbour send us an HELLO message. Check if we have a fastest route of 1 to the sender
    if (find_fastest_route(sender_mip).cost != 1) {
        // This is a new neighbour. Add a route to the sender with a cost of 1
        add_update_route(sender_mip, sender_mip, 1);
        global_debug("Added %d as a new neighbour\n", sender_mip);
        print_routing_table();
    }
    // Send an UPDATE message to all neighbours, needs to get the new neighbour updated
    send_update_messages(usd);
}


/**
 * Sends a 'HELLO' message in the MIP protocol.
 *
 * @param usd: Integer representing the file descriptor of the unix socket used for routing communication.
 *
 */
void send_hello_message(int usd) {
    hello_message message;
    message.header.mip_addr = MIP_BROADCAST;
    message.header.ttl = BROADCAST_TTL;
    message.header.id1 = 0x48; // H
    message.header.id2 = 0x45; // E
    message.header.id3 = 0x4C; // L

    // Send the HELLO message
    global_debug("Sending HELLO message\n");
    if (send(usd, &message, sizeof(message), 0) == -1) {
        perror("send");
    }
}