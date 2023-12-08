#include <string.h>
#include <sys/socket.h>
#include "upper.h"
#include "../mipd_common.h"

/**
 * Handles a client request on a Unix Socket Descriptor (USD).
 *
 * @param fds: Pointer to a file descriptor set where the function stores the accepted USDs and updates information about the number of accepted UNIX sockets.
 *
 * @return: SUCCESSES:
 *   - The function returns the value of the accepted UNIX Socket Descriptor in case of successful handling of a request.
 * ERRORS:
 *   - Returns -1 if there were any errors with 'accept' or 'recv' function calls;
 *   - Returns -2 if received ARP request on a UNIX socket;
 *   - Returns -3 if received an unknown request;
 *   - Returns -4 if maximum number of accepted UNIX sockets were reached.
 */
int handle_usd_request(struct fds *fds) {
    int accept_usd = accept(fds->usd, NULL, NULL);
    if (accept_usd < 0) {
        global_debug("accept");
        return -1;
    }

    // Receive identifier
    char buf[1024];
    long rc = recv(accept_usd, buf, sizeof(buf), 0);
    if (rc < 0) {
        global_debug("recv");
        return -1;
    }

    struct accepted_usd identified_usd;
    identified_usd.usd = accept_usd;
    u_int8_t sdu_type = buf[0];
    switch (sdu_type) {
        case MIP_SDU_TYPE_PING:
            identified_usd.type = MIP_SDU_TYPE_PING;
            fds->ping_usd = accept_usd;
            break;
        case MIP_SDU_TYPE_ARP:
            //global_debug("Got ARP request on UNIX socket");
            return -2;
        case MIP_SDU_TYPE_ROUTING:
            identified_usd.type = MIP_SDU_TYPE_ROUTING;
            fds->routing_usd = accept_usd;
            break;
        default:
            global_debug("Unknown request");
            return -3;
    }
    if (fds->num_accepted_usds >= MAX_ACCEPTED_USDS) {
        global_debug("Too many accepted unix sockets");
        return -4;
    }
    fds->accepted_usds[fds->num_accepted_usds] = identified_usd;
    fds->num_accepted_usds++;
    return accept_usd;
}

/**
 * Handles an event on an already accepted Unix Socket Descriptor.
 *
 * @param fds: A struct containing file descriptors.
 * @param ifs_data: A struct containing information about network interfaces.
 * @param accepted_usd: Accepted Unix Socket Descriptor (usd) which type identifies what actions needs to be performed.
 *
 * @return: Returns 0 in most cases, as the function usually handles the errors internally and ensures the main program continues its execution.
 * Only when an EOF (End of File) is received, it returns -2 to signal a closed connection.
 */
int handle_usd_event(struct fds fds, struct ifs_data ifs_data, struct accepted_usd accepted_usd) {
    // Receive the unix message and send it to the correct handler
    char buf[1024];
    struct unix_message *unix_message;
    long rc = recv(accepted_usd.usd, buf, sizeof(buf), 0);
    if (rc < 0) {
        global_debug("recv");
        return 0;
    } else if (rc == 0) {
        global_debug("EOF received");
        return -2;
    }

    if (accepted_usd.type == MIP_SDU_TYPE_ROUTING) {
        //global_debug("Received routing message");
        // Check for response routing message
        general_message *general_message = (struct general_message *)buf;
        //global_debug("ID1: %c", general_message->header.id1);
        //global_debug("ID2: %c", general_message->header.id2);
        //global_debug("ID3: %c", general_message->header.id3);
        if (general_message->header.id1 == 0x52 && // R
                general_message->header.id2 == 0x53 && // S
                general_message->header.id3 == 0x50) { // P
            response_message const *response_message = (struct response_message *)general_message;
            //global_debug("Got routing response");
            int err = receive_routing_response(fds, ifs_data, *response_message);
            if (err == -1) {
                global_debug("Failed to receive routing response");
                return 0;
            } else if (err == -2) {
                global_debug("Found no route");
                return 0;
            }
            return 0;
        }
    }

    // Parse the buffer into a ping packet struct
    unix_message = (struct unix_message *)buf;

    if (unix_message->ttl == 0) {
        unix_message->ttl = MIP_MAX_TTL;
    }

    // Create the MIP PDU
    struct mip_pdu mip_pdu;
    mip_pdu.sdu_type = accepted_usd.type;
    mip_pdu.sdu_len = sizeof(unix_message->sdu);
    mip_pdu.src_addr = ifs_data.local_mip_addr;
    mip_pdu.dest_addr = unix_message->mip_addr;
    mip_pdu.ttl = unix_message->ttl;
    memcpy(mip_pdu.sdu, unix_message->sdu, mip_pdu.sdu_len);

    // Send the MIP SDU
    int err = send_mip_packet(fds, ifs_data, mip_pdu);
    if (err < 0) {
        global_debug("Error sending MIP packet");
        return 0;
    } else {
        return 0;
    }
}

/**
 * Sends a PING message to the upper layer.
 *
 * @param fds: A struct containing file descriptors, uses the ping Unix Socket Descriptor for sending the message.
 * @param mip_pdu: The MIP_PDU to be sent, contains the ping message.
 */
void send_ping_message(struct fds fds,  struct mip_pdu mip_pdu) {
    send_usd_message(fds.ping_usd, mip_pdu);
}

/**
 * Sends a MIP packet to an already accepted Unix Socket Descriptor.
 *
 * @param usd: The Unix Socket Descriptor (usd) on which the message is to be sent.
 * @param mip_pdu: Mobile IP Protocol Data Unit to be sent.
 *
 * @return: Always returns 0 in the current implementation. Errors, if they occur during 'send' function call, are handled within the function with
 *          the use of 'global_debug' for logging, and the function ensures the main program execution isn't interrupted due to these errors.
 */
int send_usd_message(int usd, struct mip_pdu mip_pdu) {
    struct unix_message unix_message;
    unix_message.mip_addr = mip_pdu.src_addr;
    unix_message.ttl = mip_pdu.ttl;
    memcpy(unix_message.sdu, mip_pdu.sdu, sizeof(unix_message.sdu));

    long rc = send(usd, &unix_message, sizeof(struct unix_message), 0);
    if (rc < 0) {
        global_debug("Error sending message on UNIX socket");
        return 0;
    }
    return 0;
}