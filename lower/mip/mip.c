#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h>
#include <linux/if_packet.h>
#include "mip.h"
#include "queue.h"
#include "../../common.h"
#include "../arp/cache.h"
#include "../arp/arp.h"
#include "../../upper/upper.h"


int send_packet(int rsd, struct ifs_data ifs_data, u_int8_t *sdu, int sdu_len, u_int8_t *dest_mac, u_int8_t dest_if){
    // Create the ethernet frame
    struct ether_frame frame_hdr;
    struct msghdr	   *msghdr;
    struct iovec	   msgvec[2];
    int rc;

    memcpy(frame_hdr.src_addr, ifs_data.addr[dest_if].sll_addr, 6);
    memcpy(frame_hdr.dst_addr, dest_mac, 6);
    frame_hdr.eth_proto[0] = ETH_P_MIP >> 8;  // High byte
    frame_hdr.eth_proto[1] = ETH_P_MIP & 0xFF;  // Low byte
    //global_debug("Sending MIP-packet from %02X:%02X:%02X:%02X:%02X:%02X to %02X:%02X:%02X:%02X:%02X:%02X on interface %d",
    //             frame_hdr.src_addr[0], frame_hdr.src_addr[1], frame_hdr.src_addr[2], frame_hdr.src_addr[3], frame_hdr.src_addr[4], frame_hdr.src_addr[5],
    //            frame_hdr.dst_addr[0], frame_hdr.dst_addr[1], frame_hdr.dst_addr[2], frame_hdr.dst_addr[3], frame_hdr.dst_addr[4], frame_hdr.dst_addr[5],
    //             dest_if);


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
        perror("sendmsg() failed");
        free(msghdr);
        return -2;
    }

    free(msghdr);

    return rc;
}

int send_mip_packet(int rsd, struct ifs_data ifs_data, struct mip_pdu mip_pdu){
    mip_pdu.ttl--;
    //global_debug("Sending MIP packet from %d to %d", src_addr, mip_pdu.dest_addr);

    // Check if the destination address is in the arp cache
    struct arp_cache_entry *cache_entry = arp_cache_get(mip_pdu.dest_addr);
    if (cache_entry == NULL) {
        global_debug("No MAC address found in arp-cache for MIP address %d", mip_pdu.dest_addr);
        int err = send_arp_request(rsd, ifs_data, mip_pdu.dest_addr);
        if (err < 0) {
            global_debug("Failed to send ARP request");
            return -1;
        }
        global_debug("Sent ARP, adding MIP packet to queue");
        err = enqueue_mip_pdu(&mip_pdu);
        if (err < 0) {
            global_debug("Failed to enqueue MIP packet");
            return -1;
        }
        return 0;
    }

    // Destination address was found in the arp cache, send the packet
    global_debug("---------------------------");
    global_debug("Sending MIP packet:");
    global_debug("MIP: %d -> %d", mip_pdu.src_addr, mip_pdu.dest_addr);
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
    int rc = send_packet(rsd, ifs_data, (u_int8_t *)&mip_pdu, sizeof(struct mip_pdu), cache_entry->mac_addr, cache_entry->interface);
    if (rc < 0) {
        //global_debug("Failed to send MIP packet");
        return -1;
    }
    return rc;

}


int send_broadcast_packet(int rsd, struct ifs_data ifs_data, u_int8_t *sdu, int sdu_len) {
    // Create the ethernet frame
    struct ether_frame frame_hdr;
    struct msghdr	   *msghdr;
    struct iovec	   msgvec[2];
    int	   rc = 0;


    //global_debug("Sending broadcast packet");
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
        //global_debug("Sending broadcast packet on interface %d", ifi);
        memcpy(frame_hdr.src_addr, ifs_data.addr[ifi].sll_addr, 6);
        msghdr->msg_name	 = &(ifs_data.addr[ifi]);

        rc += sendmsg(rsd, msghdr, 0);
        if (rc < 0) {
            perror("sendmsg() failed");
            free(msghdr);
            return -2;
        }
    }

    free(msghdr);

    return rc;
}

int handle_mip_packet(int rsd, int usd, struct msghdr msghdr, struct ifs_data ifs_data) {
    struct mip_pdu mip_pdu = *((struct mip_pdu *)msghdr.msg_iov[1].iov_base);
    // Check if the packet is for us
    if (mip_pdu.dest_addr != ifs_data.local_mip_addr && mip_pdu.dest_addr != MIP_BROADCAST_ADDR) {
        global_debug("Received MIP-packet not for us, dropping");
        return 0;
    }

    // Check SDU-type
    switch (mip_pdu.sdu_type) {
        case MIP_SDU_TYPE_ARP:
            global_debug("Received ARP packet");
            return handle_arp_packet(rsd, ifs_data, msghdr);

        case MIP_SDU_TYPE_PING:
            global_debug("Received PING packet");
            return handle_ping(usd, ifs_data, msghdr);

        default:
            global_debug("Received MIP packet with unknown type: %d", mip_pdu.sdu_type);
            return -1;
    }
}

int check_mip_queue(int rsd, u_int8_t mip_addr, struct ifs_data ifs_data) {
    global_debug("Checking MIP queue for MIP address %d", mip_addr);
    struct mip_pdu *mip_pdu = dequeue_mip_pdu(mip_addr);
    if (mip_pdu == NULL) {
        global_debug("No MIP packet in queue for MIP address %d", mip_addr);
        return 0;
    }
    global_debug("Found MIP packet in queue for MIP address %d", mip_addr);

    int err = send_mip_packet(rsd, ifs_data, *mip_pdu);
    if (err < 0) {
        global_debug("Failed to send MIP packet");
        return -1;
    }

    return 0;
}