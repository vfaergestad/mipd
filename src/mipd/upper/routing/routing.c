#include <sys/socket.h>
#include "routing.h"
#include "../../mipd_common.h"

/**
 * Forwarding a routing message to routingd.
 *
 * @param fds: A set of file descriptors, containing various Unix Socket Descriptors that the function
 * uses to communicate with other parts of the system.
 *
 * @param mip_pdu: An instance of a  MIP PDU that needs to be forwarded to the routing daemon.
 *
 * @return: This function doesn't return any value. Errors during the message sending are assumed to be handled by the 'send_usd_message' function.
 */
void forward_routing_message(struct fds fds, struct mip_pdu mip_pdu) {
    global_debug("Forwarding routing message to routingd \n");
    send_usd_message(fds.routing_usd, mip_pdu);
}

/**
 * Sends a routing request message to a specified MIP address.
 *
 * @param usd: The Unix Socket Descriptor used for sending the message.
 * @param ifs_data: Contains information about network interfaces, includes the local MIP address that is used in the header of the request message.
 * @param dest_addr: The destination MIP address to which a route is to be found.
 *
 * @return: 0 if the request is sent successfully; -1 if an error occurs during the send operation.
 */
int send_routing_request(int usd, struct ifs_data ifs_data, u_int8_t dest_addr) {
    global_debug("Sending routing request for %d\n", dest_addr);
    request_message request;
    request.header.mip_addr = ifs_data.local_mip_addr;
    request.header.ttl = 0;
    request.header.id1 = 0x52; // R
    request.header.id2 = 0x45; // E
    request.header.id3 = 0x51; // Q
    request.mip_look_up = dest_addr; // The MIP address we want to find a route to

    long rc = send(usd, &request, sizeof(request_message), 0);
    if (rc < 0) {
        global_debug("send");
        return -1;
    }
    return 0;
}