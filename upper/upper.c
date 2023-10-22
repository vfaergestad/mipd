#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include "upper.h"
#include "../common.h"
#include "../lower/mip/mip.h"

int handle_usd_event(struct fds fds, struct ifs_data ifs_data) {
    int usd = fds.usd_ping;
    int rsd = fds.rsd;
    char buf[1024];
    int dest_mip_addr;
    struct ping_packet *ping_packet;
    int rc = recv(usd, buf, sizeof(buf), 0);
    if (rc < 0) {
        perror("recv");
        return -1;
    } else if (rc == 0) {
        //global_debug("EOF received");
        return -2;
    } else {
        // Parse the buffer into a ping packet struct
        ping_packet = (struct ping_packet *)buf;
        global_debug("Received ping/pong packet to MIP address %d", ping_packet->mip_addr);

        // Storing the destination MIP address
        dest_mip_addr = ping_packet->mip_addr;
    }

    // Create the MIP PDU
    struct mip_pdu mip_pdu;
    mip_pdu.sdu_type = MIP_SDU_TYPE_PING;
    mip_pdu.sdu_len = sizeof(ping_packet->message);
    mip_pdu.src_addr = ifs_data.local_mip_addr;
    mip_pdu.dest_addr = dest_mip_addr;
    mip_pdu.ttl = MIP_MAX_TTL;
    memcpy(mip_pdu.sdu, ping_packet->message, mip_pdu.sdu_len);

    // Send the MIP SDU
    rc = send_mip_packet(rsd, ifs_data, mip_pdu);
    if (rc < 0) {
        global_debug("Error sending MIP packet");
        return -1;
    } else {
        return 0;
    }
}

int handle_ping(int usd, struct ifs_data ifs_data, struct msghdr msghdr) {
    // Extract the MIP PDU from msghdr
    struct mip_pdu recv_mip_pdu = *((struct mip_pdu *)msghdr.msg_iov[1].iov_base);
    global_debug("Handling ping packet from MIP address %d to MIP address %d", recv_mip_pdu.src_addr, recv_mip_pdu.dest_addr);

    // Extract the ping packet from the MIP PDU's sdu field
    struct ping_packet extracted_ping;

    memcpy(&extracted_ping.message, recv_mip_pdu.sdu, sizeof(extracted_ping.message));
    extracted_ping.mip_addr = recv_mip_pdu.src_addr;

    // Send the extracted ping packet upwards through the Unix socket
    int rc = send(usd, &extracted_ping, sizeof(struct ping_packet), 0);
    if (rc < 0) {
        perror("send");
        return -1;
    }
    return 0;
}
