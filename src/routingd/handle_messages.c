#include <string.h>
#include "../common/routing/routing_messages.h"
#include "hello/hello.h"
#include "update/update.h"
#include "request/request.h"
#include "routing_common.h"


/**
 * Function for handling incoming routing protocol messages.
 *
 * @param usd: Integer representing the file descriptor of the unix socket
 *             used for routing communication.
 * @param message: The general_message structure received from another node.
 */
void handle_message(int usd, general_message message) {
    u_int8_t id1 = message.header.id1;
    u_int8_t id2 = message.header.id2;
    u_int8_t id3 = message.header.id3;
    //global_debug("Received message with ID %c%c%c\n", id1, id2, id3);

    // Identify HELLO, UPDATE, REQUEST
    if (id1 == 0x48 && id2 == 0x45 && id3 == 0x4c) {
        // HELLO
        hello_message hello;
        memcpy(&hello, &message, sizeof(hello));
        handle_hello_message(usd, hello);
    } else if (id1 == 0x55 && id2 == 0x50 && id3 == 0x44) {
        // UPDATE
        update_message update;
        memcpy(&update, &message, sizeof(update));
        handle_update_message(usd, update);
    } else if (id1 == 0x52 && id2 == 0x45 && id3 == 0x51) {
        // REQUEST
        request_message request;
        memcpy(&request, &message, sizeof(request));
        handle_request_message(usd, request);
    } else {
        global_debug("Unknown message ID\n");
    }
}
