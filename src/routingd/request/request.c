#include <stdio.h>
#include <sys/socket.h>
#include "request.h"
#include "../routing_common.h"

/**
 * Sends a 'RESPONSE' message in the MIP protocol.
 *
 * This function is responsible for constructing a 'RESPONSE' message in response to a 'REQUEST'
 * message and sending it through the provided unix socket descriptor (usd).
 *
 * @param usd: Integer representing the file descriptor of the unix socket used for routing communication.
 * @param request: The 'REQUEST' message that was received.
 * @param next_hop: The MIP address of the next hop node.
 */
void send_response_message(int usd, request_message request, u_int8_t next_hop) {
    response_message response;
    response.header.mip_addr = request.header.mip_addr;
    response.header.ttl = 0;
    response.header.id1 = 0x52; // R
    response.header.id2 = 0x53; // S
    response.header.id3 = 0x50; // P
    response.next_hop_mip = next_hop;

    // Send the RESPONSE message
    global_debug("Sending RESPONSE message: next hop is %d\n", next_hop);
    if (send(usd, &response, sizeof(response), 0) == -1) {
        perror("send");
    }

}

/**
 * Processes 'REQUEST' messages in the MIP protocol.
 *
 * This function handles the parsing and response for incoming 'REQUEST' messages.
 * Given such a message, it finds the fastest route to the requested MIP.
 * If no route is found, it designates the next hop as 255.
 *
 * @param usd: The file descriptor of the unix socket used for routing communication.
 * @param message: The 'REQUEST' message that was received.
 */
void handle_request_message(int usd, request_message message) {
    u_int8_t mip_look_up = message.mip_look_up;
    u_int8_t next_hop;

    global_debug("Received REQUEST for MIP %d\n", mip_look_up);
    route_info fastest_route = find_fastest_route(mip_look_up);
    if (fastest_route.next_hop == 0) {
        global_debug("No route found");
        next_hop = 255;
    } else {
        next_hop = fastest_route.next_hop;
    }
    send_response_message(usd, message, next_hop);
}
