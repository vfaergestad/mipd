#include <stdio.h> 		/* printf */
#include <stdlib.h>		/* free */
#include <unistd.h>             /* fgets */
#include <string.h>		/* memset */
#include <sys/socket.h>		/* socket */
#include <sys/epoll.h>          /* epoll */
#include <linux/if_packet.h>	/* AF_PACKET */
#include <net/ethernet.h>	/* ETH_* */
#include <arpa/inet.h>		/* htons */
#include <ifaddrs.h>		/* getifaddrs */
#include "util.h"
#include "../common.h"
#include "mip/mip.h"
#include "arp/arp.h"

/**
 * Handles and processes events that are received by the raw socket 
 * which is used for sending and receiving MIP packets.
 *
 * @param fds: A struct that contains multiple file descriptors including raw and unix sockets.
 * @param ifs_data: A struct that contains information about network interfaces.
 *
 * The function receives the ethernet frame from the raw socket and checks if the frame type is MIP. 
 * It also checks if the frame is destined for the running node by comparing the destination MAC address 
 * with the MAC addresses of the node's network interfaces. 
 * If the received packet is a MIP packet and is destined for us, the function calls handle_mip_packet() to 
 * handle the MIP packet.
 *
 * @return: 0 if the event is processed successfully;
 * -1 if an error occurs such as failure in receiving message or error in handling MIP packet.
 */
int handle_rsd_event(struct fds fds, struct ifs_data ifs_data) {
    // Retrieve the relevant socket descriptors from fds struct.
    int rsd = fds.rsd;
    int usd = fds.usd_ping;

    // Prepare the structures for recieving the frame from the raw socket.
    struct sockaddr_ll so_name;
    struct ether_frame frame_hdr;
    struct msghdr      msghdr;
    struct iovec       msgvec[2];
    int 	  	       rc;

    struct mip_pdu mip_pdu;

    msgvec[0].iov_base = &frame_hdr;
    msgvec[0].iov_len  = sizeof(struct ether_frame);
    msgvec[1].iov_base = &mip_pdu;
    msgvec[1].iov_len  = sizeof(struct mip_pdu);

    memset(&msghdr, 0, sizeof(struct msghdr));
    msghdr.msg_name = &so_name;
    msghdr.msg_namelen = sizeof(struct sockaddr_ll);
    msghdr.msg_iov = msgvec;
    msghdr.msg_iovlen = 2;

    // Receive frame from raw socket.
    rc = recvmsg(rsd, &msghdr, 0);
    if (rc < 0) {
        perror("recvmsg() failed");
        return -1;
    }

    global_debug("Received frame from %02X:%02X:%02X:%02X:%02X:%02X to %02X:%02X:%02X:%02X:%02X:%02X",
                 frame_hdr.src_addr[0], frame_hdr.src_addr[1], frame_hdr.src_addr[2], frame_hdr.src_addr[3], frame_hdr.src_addr[4], frame_hdr.src_addr[5],
                 frame_hdr.dst_addr[0], frame_hdr.dst_addr[1], frame_hdr.dst_addr[2], frame_hdr.dst_addr[3], frame_hdr.dst_addr[4], frame_hdr.dst_addr[5]);


    // Loop through local interfaces to check if the frame is for us. Frame is for us is the MAC-address exsists in ifs_data or is the broadcast address.
    uint8_t broadcast_addr[] = MIP_BROADCAST_MAC_ADDR;
    int match = 0;
    int i;
    for (i = 0; i < ifs_data.ifn; i++) {
        if (memcmp(frame_hdr.dst_addr, ifs_data.addr[i].sll_addr, 6) == 0 || memcmp(frame_hdr.dst_addr, broadcast_addr, 6) == 0) {
            match = 1; // Found match.
            break;
        }
    }

    // Packet not for this node.
    if (!match) {
        //global_debug("Received ethernet-frame not for this node, dropping");
        return 0;
    }

    // Check if the packet is a MIP-packet
    if ((frame_hdr.eth_proto[0] == (ETH_P_MIP >> 8)) && (frame_hdr.eth_proto[1] == (ETH_P_MIP & 0xFF))) {
        // Send frame to the MIP handle.
        int err = handle_mip_packet(rsd, usd, msghdr, ifs_data);
        if (err < 0) {
            global_debug("Error handling MIP packet");
            return -1;
        }
        return 0;
    } else {
        global_debug("Received non-MIP-packet, dropping");
        return 0;
    }
}