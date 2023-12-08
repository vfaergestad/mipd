#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h>
#include <linux/if_packet.h>
#include "mip.h"
#include "queues/arp_queue.h"
#include "../../mipd_common.h"
#include "../arp/cache.h"
#include "../arp/arp.h"
#include "queues/route_queue.h"
#include "../../upper/routing/routing.h"

/**
 * Sends a packet over the network using a specified Ethernet frame format.
 *
 * @param rsd: Raw socket descriptor used for sending the packet.
 * @param ifs_data: Structure containing information about network interfaces.
 * @param sdu: Pointer to the Service Data Unit (SDU) to be sent.
 * @param sdu_len: Length of the SDU in bytes.
 * @param dest_mac: Pointer to the destination MAC address.
 * @param dest_if: Interface index indicating which network interface to use for sending the packet.
 *
 * Global variables:
 * - ETH_P_MIP: Ethertype for MIP packets, defining how the Ethernet protocol field is set in the frame header.
 *
 * @return: Returns the number of bytes sent on success; returns -2 if the send operation fails.
 */
int send_packet(int rsd, struct ifs_data ifs_data, u_int8_t *sdu, int sdu_len, const u_int8_t *dest_mac, u_int8_t dest_if){
    // Create the ethernet frame
    struct ether_frame frame_hdr;
    struct msghdr	   *msghdr;
    struct iovec	   msgvec[2];
    long rc;

    memcpy(frame_hdr.src_addr, ifs_data.addr[dest_if].sll_addr, 6);
    memcpy(frame_hdr.dst_addr, dest_mac, 6);
    frame_hdr.eth_proto[0] = ETH_P_MIP >> 8;  // High byte
    frame_hdr.eth_proto[1] = ETH_P_MIP & 0xFF;  // Low byte
    global_debug("Sending MIP-packet from %02X:%02X:%02X:%02X:%02X:%02X to %02X:%02X:%02X:%02X:%02X:%02X on interface %d",
                 frame_hdr.src_addr[0], frame_hdr.src_addr[1], frame_hdr.src_addr[2], frame_hdr.src_addr[3], frame_hdr.src_addr[4], frame_hdr.src_addr[5],
                frame_hdr.dst_addr[0], frame_hdr.dst_addr[1], frame_hdr.dst_addr[2], frame_hdr.dst_addr[3], frame_hdr.dst_addr[4], frame_hdr.dst_addr[5],
                 dest_if);


    msgvec[0].iov_base = &frame_hdr;
    msgvec[0].iov_len  = sizeof(struct ether_frame);
    msgvec[1].iov_base = sdu;
    msgvec[1].iov_len  = sdu_len;

    msghdr = (struct msghdr *)calloc(1, sizeof(struct msghdr));
    msghdr->msg_namelen = sizeof(struct sockaddr_ll);
    msghdr->msg_iovlen	 = 2;
    msghdr->msg_iov	 = msgvec;
    msghdr->msg_name	 = &(ifs_data.addr[dest_if]);

    rc = sendmsg(rsd, msghdr, 0);
    if (rc < 0) {
        free(msghdr);
        return -2;
    }

    free(msghdr);

    return rc;
}

/**
 * Sends a MIP packet, handling both direct broadcast and routed packets.
 *
 * @param fds: File descriptor structure containing raw and UNIX socket descriptors.
 * @param ifs_data: Structure containing information about network interfaces.
 * @param mip_pdu: The MIP Protocol Data Unit to be sent.
 *
 * Global variables:
 * - MIP_BROADCAST_ADDR: Address used to identify broadcast packets in the MIP protocol.
 *
 * @return: Returns 0 on successful packet transmission or queuing; returns -1 for any failures in sending or queuing.
 *
 */
int send_mip_packet(struct fds fds, struct ifs_data ifs_data, struct mip_pdu mip_pdu){
    global_debug("Trying to MIP packet from %d to %d", mip_pdu.src_addr, mip_pdu.dest_addr);

    // Check if the destination address is broadcast
    if (mip_pdu.dest_addr == MIP_BROADCAST_ADDR) {
        global_debug("Sending broadcast MIP packet");
        int err = send_broadcast_packet(fds.rsd, ifs_data, (u_int8_t *)&mip_pdu, sizeof(struct mip_pdu));
        if (err < 0) {
            global_debug("Failed to send broadcast MIP packet");
            return -1;
        }
        return 0;
    }

    // Ask routing daemon for next hop, queue packet in routing queue.
    //global_debug("Sending routing request");
    int err = send_routing_request(fds.routing_usd, ifs_data, mip_pdu.dest_addr);
    if (err != 0) {
        global_debug("Error while sending routing request");
        return -1;
    }
    global_debug("Routing request sent, adding MIP packet to route queue");
    err = route_enqueue(mip_pdu);
    if (err != 0) {
        global_debug("Error while enqueing mip_pdu to route queue");
        return -1;
    }
    return 0;
}

/**
 * Sends a broadcast packet over all network interfaces using Ethernet broadcast addressing.
 *
 * @param rsd: Raw socket descriptor used for sending the packet.
 * @param ifs_data: Structure containing information about network interfaces.
 * @param sdu: Pointer to the Service Data Unit (SDU) to be sent.
 * @param sdu_len: Length of the SDU in bytes.
 *
 * Global variables:
 * - MIP_BROADCAST_MAC_ADDR: MAC address used for Ethernet broadcasting.
 * - ETH_P_MIP: Ethertype for MIP packets, used in setting the Ethernet frame's protocol field.
 *
 * @return: The total number of bytes sent across all interfaces; returns -2 if any send operation fails.
 *
 */
int send_broadcast_packet(int rsd, struct ifs_data ifs_data, u_int8_t *sdu, int sdu_len) {
    // Create the ethernet frame
    struct ether_frame frame_hdr;
    struct msghdr	   *msghdr;
    struct iovec	   msgvec[2];
    int	   rc = 0;


    global_debug("Sending broadcast packet");
    uint8_t dest_addr[] = MIP_BROADCAST_MAC_ADDR;
    memcpy(frame_hdr.dst_addr, dest_addr, 6);
    frame_hdr.eth_proto[0] = ETH_P_MIP >> 8;  // High byte
    frame_hdr.eth_proto[1] = ETH_P_MIP & 0xFF;  // Low byte

    msgvec[0].iov_base = &frame_hdr;
    msgvec[0].iov_len  = sizeof(struct ether_frame);
    msgvec[1].iov_base = sdu;
    msgvec[1].iov_len  = sdu_len;

    msghdr = (struct msghdr *)calloc(1, sizeof(struct msghdr));
    msghdr->msg_namelen = sizeof(struct sockaddr_ll);
    msghdr->msg_iovlen	 = 2;
    msghdr->msg_iov	 = msgvec;

    // Destination address was found in the arp cache, send the packet
    for (int ifi = 0; ifi < ifs_data.ifn; ifi++) {
        global_debug("---------------------------");
        global_debug("Sending broadcast MIP packet:");
        global_debug("MIP: %d -> %d", ifs_data.local_mip_addr, MIP_BROADCAST_ADDR);
        global_debug("MAC: %02X:%02X:%02X:%02X:%02X:%02X -> %02X:%02X:%02X:%02X:%02X:%02X",
                     ifs_data.addr[ifi].sll_addr[0],
                     ifs_data.addr[ifi].sll_addr[1],
                     ifs_data.addr[ifi].sll_addr[2],
                     ifs_data.addr[ifi].sll_addr[3],
                     ifs_data.addr[ifi].sll_addr[4],
                     ifs_data.addr[ifi].sll_addr[5],
                     frame_hdr.dst_addr[0],
                     frame_hdr.dst_addr[1],
                     frame_hdr.dst_addr[2],
                     frame_hdr.dst_addr[3],
                     frame_hdr.dst_addr[4],
                     frame_hdr.dst_addr[5]);
        global_debug("---------------------------");
        global_debug("Current content of ARP cache:");
        arp_cache_print_to_debug();
        global_debug("---------------------------");
        global_debug("Sending broadcast packet on interface %d", ifi);
        memcpy(frame_hdr.src_addr, ifs_data.addr[ifi].sll_addr, 6);
        msghdr->msg_name	 = &(ifs_data.addr[ifi]);

        rc += sendmsg(rsd, msghdr, 0);
        if (rc < 0) {
            global_debug("sendmsg() failed");
            free(msghdr);
            return -2;
        }
    }

    free(msghdr);

    return rc;
}

/**
 * Checks the ARP queue for any packets waiting to be sent to a specified next hop and sends them.
 *
 * @param fds: File descriptor structure containing raw and UNIX socket descriptors.
 * @param next_hop: The MIP address of the next hop to which packets should be sent.
 * @param ifs_data: Structure containing information about network interfaces.
 *
 * @return: Returns 0 if there are no packets for the specified next hop or if packets are successfully sent;
 *          returns -1 if there is a failure in sending a packet.
 *
 */
int check_arp_queue(struct fds fds, u_int8_t next_hop, struct ifs_data ifs_data) {
    //global_debug("Checking MIP queue for MIP address %d", next_hop);
    struct mip_pdu const *mip_pdu = arp_dequeue_mip_pdu(next_hop);
    if (mip_pdu == NULL) {
        global_debug("No MIP packet in queue for MIP address %d", next_hop);
        return 0;
    }
    //global_debug("Found MIP packet in queue for MIP address %d", next_hop);

    int err = send_mip_packet(fds, ifs_data, *mip_pdu);
    if (err < 0) {
        global_debug("Failed to send MIP packet");
        return -1;
    }

    return 0;
}

/**
 * Processes a routing response and handles the forwarding of packets based on the received route information.
 *
 * @param fds: File descriptor structure containing raw and UNIX socket descriptors.
 * @param ifs_data: Structure containing information about network interfaces.
 * @param response: The routing response message indicating the next hop for a queued MIP packet.
 *
 * @return: Returns 0 if the process is successful or if no route is found (packet dropped);
 *          returns -1 for failures in sending ARP requests or MIP packets;
 *          returns -2 if the routing queue is empty when a routing response is received.
 *
 */
int receive_routing_response(struct fds fds, struct ifs_data ifs_data, response_message response) {
    // Check if routing queue is not empty
    if (is_route_queue_empty() == 1) {
        global_debug("Got a routing response, but the routing queue is empty.");
        return -2;
    }
    u_int8_t next_hop = response.next_hop_mip;
    // Check if no route was found
    if (next_hop == 255) {
        global_debug("No route found, dropping one packet from the routing queue");
        route_dequeue();
        return 0;
    }
    // Get next mip_pdu from routing queue
    global_debug("Dequeueing MIP packet from routing queue");
    struct mip_pdu mip_pdu = route_dequeue();

    // Check if the destination address is in the arp cache
    struct arp_cache_entry const *cache_entry = arp_cache_get(next_hop);
    if (cache_entry == NULL) {
        global_debug("No MAC address found in arp-cache for MIP address %d", next_hop);
        int err = send_arp_request(fds.rsd, ifs_data, next_hop);
        if (err < 0) {
            global_debug("Failed to send ARP request");
            return -1;
        }
        global_debug("Sent ARP, adding MIP packet to queue");
        err = arp_enqueue_mip_pdu(&mip_pdu, next_hop);
        if (err < 0) {
            global_debug("Failed to route_enqueue MIP packet");
            return -1;
        }
        return 0;
    }

    // Destination address was found in the arp cache, send the packet
    global_debug("---------------------------");
    global_debug("Sending MIP packet:");
    global_debug("MIP: %d -> %d, via %d", mip_pdu.src_addr, mip_pdu.dest_addr, next_hop);
    global_debug("MAC: %02X:%02X:%02X:%02X:%02X:%02X -> %02X:%02X:%02X:%02X:%02X:%02X",
                 ifs_data.addr[0].sll_addr[0],
                 ifs_data.addr[0].sll_addr[1],
                 ifs_data.addr[0].sll_addr[2],
                 ifs_data.addr[0].sll_addr[3],
                 ifs_data.addr[0].sll_addr[4],
                 ifs_data.addr[0].sll_addr[5],
                 cache_entry->mac_addr[0],
                 cache_entry->mac_addr[1],
                 cache_entry->mac_addr[2],
                 cache_entry->mac_addr[3],
                 cache_entry->mac_addr[4],
                 cache_entry->mac_addr[5]);
    global_debug("---------------------------");
    global_debug("Current content of ARP cache:");
    arp_cache_print_to_debug();
    global_debug("---------------------------");
    int rc = send_packet(fds.rsd, ifs_data, (u_int8_t *)&mip_pdu, sizeof(struct mip_pdu), cache_entry->mac_addr, cache_entry->interface);
    if (rc < 0) {
        global_debug("Failed to send MIP packet");
        return -1;
    }
    return rc;
}