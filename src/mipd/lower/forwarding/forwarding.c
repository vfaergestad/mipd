#include <sys/socket.h>
#include "../../mipd_common.h"
#include "forwarding.h"
#include "../arp/arp.h"
#include "../../upper/routing/routing.h"

/**
 * Forwards a MIP PDU based on its destination and SDU type.
 *
 * @param fds: File descriptor structure, containing raw and UNIX socket descriptors.
 * @param ifs_data: Network interface data structure, including local MIP address information.
 * @param mip_pdu: The MIP PDU to be forwarded.
 * @param msghdr: Message header structure containing the received packet information.
 *
 * The function first checks if the destination address of the MIP PDU is the local host or a broadcast address.
 * If so, it handles the PDU locally based on its SDU type (e.g., PING, ROUTING, ARP). If the destination is not local,
 * it decrements the TTL (Time To Live) and checks if it has expired. If TTL is expired, the packet is dropped. Otherwise,
 * the function attempts to forward the packet to the next hop.
 *
 * @return: Returns 0 if the PDU is successfully processed or forwarded; returns -1 for failures, such as unknown SDU types
 *          or issues in sending the MIP packet.
 */
int forward_mip_pdu(struct fds fds, struct ifs_data ifs_data, struct mip_pdu mip_pdu, struct msghdr *msghdr){
    // Check if the destination address is the local host
    if (mip_pdu.dest_addr == ifs_data.local_mip_addr || mip_pdu.dest_addr == MIP_BROADCAST_ADDR) {
        // Handle local delivery based on the SDU type
        switch (mip_pdu.sdu_type) {
            case MIP_SDU_TYPE_PING:
                send_ping_message(fds, mip_pdu);
                break;
            case MIP_SDU_TYPE_ROUTING:
                //global_debug("Received routing message for this node");
                // Packet is either HALLO or UPDATE
                forward_routing_message(fds, mip_pdu);
                break;
            case MIP_SDU_TYPE_ARP:
                //global_debug("Received ARP packet");
                return handle_arp_packet(fds, ifs_data, msghdr);
            default:
                fprintf(stderr, "Unknown SDU type\n");
                return -1;
        }
        return 0; // No further processing needed

    } else if (--mip_pdu.ttl == 0) {
        global_debug("TTL expired");
        // DROP PACKET
        return 0;
    } else {
        // Forward the packet
        int err = send_mip_packet(fds, ifs_data, mip_pdu);
        if (err < 0) {
            global_debug("Failed to send MIP packet");
            return -1;
        }
        return 0;
    }
}