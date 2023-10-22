#include <string.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include "../../common.h"
#include "arp.h"
#include "cache.h"
#include <stdint.h>


/**
 * Sends an ARP request for the given MIP address.
 * @param rsd The raw socket descriptor to send the ARP request on.
 * @param ifs_data The interface data.
 * @param mip_pdu The MIP PDU to send.
 * @return 0 if no error occurred, -1 if an error occurred.
 */
int send_arp_request(int rsd, struct ifs_data ifs_data, u_int8_t dest_addr) {
    global_debug("Sending ARP request for MIP address %d", dest_addr);
    struct arp_message arp_msg;
    arp_msg.type = ARP_TYPE_REQUEST;
    arp_msg.mip_addr = dest_addr;
    arp_msg.padding = 0;

    struct mip_pdu mip_pdu;
    mip_pdu.src_addr = ifs_data.local_mip_addr;
    mip_pdu.sdu_type = MIP_SDU_TYPE_ARP;
    mip_pdu.dest_addr = MIP_BROADCAST_ADDR;
    mip_pdu.sdu_len = sizeof(struct arp_message);
    memcpy(mip_pdu.sdu, &arp_msg, sizeof(struct arp_message));
    mip_pdu.ttl = MIP_BROADCAST_TTL;

    int rc = send_broadcast_packet(rsd, ifs_data, (u_int8_t *)&mip_pdu, sizeof(struct mip_pdu));
    if (rc < 0) {
        perror("send_broadcast_packet() failed");
        return -1;
    }

    return rc;
}

int handle_arp_packet(int rsd, struct ifs_data ifs_data, struct msghdr msghdr) {
    struct sockaddr_ll so_name = *((struct sockaddr_ll *)msghdr.msg_name);
    struct ether_frame frame_hdr = *((struct ether_frame *)msghdr.msg_iov[0].iov_base);
    struct mip_pdu recv_mip_pdu = *((struct mip_pdu *)msghdr.msg_iov[1].iov_base);
    struct arp_message *arp_msg = (struct arp_message *)recv_mip_pdu.sdu;

    // Check if ARP message is for us
    if ((arp_msg->mip_addr != ifs_data.local_mip_addr) && (recv_mip_pdu.dest_addr != ifs_data.local_mip_addr)) {
        global_debug("Received ARP packet not for us, dropping");
        return 0;
    }

    // Get interface index
    int ifi = get_if_index(ifs_data, so_name.sll_ifindex);
    if (ifi < 0) {
        global_debug("Interface index not found");
        return -1;
    }
    //global_debug("Interface index found: %d", ifi);

    if (arp_msg->type == ARP_TYPE_REQUEST) {
        global_debug("Received ARP request");


        // Add the MIP address to the ARP cache
        arp_cache_add(recv_mip_pdu.src_addr, frame_hdr.src_addr, ifi);

        global_debug("Sending ARP response");
        // Send an ARP response
        struct arp_message arp_response;
        arp_response.type = ARP_TYPE_RESPONSE;
        arp_response.mip_addr = arp_msg->mip_addr;
        arp_response.padding = 0;

        struct mip_pdu mip_pdu;
        mip_pdu.src_addr = ifs_data.local_mip_addr;
        mip_pdu.sdu_type = MIP_SDU_TYPE_ARP;
        mip_pdu.dest_addr = recv_mip_pdu.src_addr;
        mip_pdu.sdu_len = sizeof(struct arp_message);
        memcpy(mip_pdu.sdu, &arp_response, sizeof(struct arp_message));
        mip_pdu.ttl = MIP_MAX_TTL;

        int rc = send_mip_packet(rsd, ifs_data, mip_pdu);
        if (rc < 0) {
            global_debug("Error sending ARP response");
            return -1;
        }

    } else if (arp_msg->type == ARP_TYPE_RESPONSE) {
        global_debug("Received ARP response");
        // Add the MIP address to the ARP cache
        arp_cache_add(recv_mip_pdu.src_addr, frame_hdr.src_addr, ifi);
        // Check if there are any packets in the MIP queue for this MIP address
        check_mip_queue(rsd, arp_msg->mip_addr, ifs_data);
    } else {
        global_debug("Received ARP packet with unknown type");
        return -1;
    }

    return 0;
}