#include <string.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include "lower.h"
#include "../mipd_common.h"
#include "mip/mip.h"
#include "forwarding/forwarding.h"

/**
 * Handles and processes events that are received by the raw socket 
 * which is used for sending and receiving MIP packets.
 *
 * @param fds: A struct that contains multiple file descriptors including raw and unix sockets.
 * @param ifs_data: A struct that contains information about network interfaces.
 *
 * @return: 0 if the event is processed successfully;
 * -1 if an error occurs such as failure in receiving message or error in handling MIP packet.
 */
int handle_rsd_event(struct fds fds, struct ifs_data ifs_data) {
    // Retrieve the relevant socket descriptors from fds struct.
    int rsd = fds.rsd;

    // Prepare the structures for receiving the frame from the raw socket.
    struct sockaddr_ll so_name;
    struct ether_frame frame_hdr;
    struct msghdr      msghdr;
    struct iovec       msgvec[2];
    long               rc;

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
        global_debug("recvmsg() failed");
        return -1;
    }

    global_debug("Received frame from %02X:%02X:%02X:%02X:%02X:%02X to %02X:%02X:%02X:%02X:%02X:%02X",
                 frame_hdr.src_addr[0], frame_hdr.src_addr[1], frame_hdr.src_addr[2], frame_hdr.src_addr[3], frame_hdr.src_addr[4], frame_hdr.src_addr[5],
                 frame_hdr.dst_addr[0], frame_hdr.dst_addr[1], frame_hdr.dst_addr[2], frame_hdr.dst_addr[3], frame_hdr.dst_addr[4], frame_hdr.dst_addr[5]);


    // Loop through local interfaces to check if the frame is for us. Frame is for us is the MAC-address exists in ifs_data or is the broadcast address.
    uint8_t broadcast_addr[] = MIP_BROADCAST_MAC_ADDR;
    int match = 0;
    for (int i = 0; i < ifs_data.ifn; i++) {
        if (memcmp(frame_hdr.dst_addr, ifs_data.addr[i].sll_addr, 6) == 0 || memcmp(frame_hdr.dst_addr, broadcast_addr, 6) == 0) {
            match = 1; // Found match.
            break;
        }
    }

    // Packet not for this node.
    if (!match) {
        global_debug("Received ethernet-frame not for this node, dropping");
        return 0;
    }

    // Check if the packet is a MIP-packet
    if ((frame_hdr.eth_proto[0] == (ETH_P_MIP >> 8)) && (frame_hdr.eth_proto[1] == (ETH_P_MIP & 0xFF))) {
        // Send frame to the MIP forwarder.
        //global_debug("Received MIP-packet, forwarding to MIP forwarder");
        int err = forward_mip_pdu(fds, ifs_data, mip_pdu, &msghdr);
        if (err < 0) {
            global_debug("Error handling MIP packet");
            return 0;
        }
        return 0;
    } else {
        global_debug("Received non-MIP-packet, dropping");
        return 0;
    }
}