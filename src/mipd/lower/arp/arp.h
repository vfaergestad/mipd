#ifndef ARP_H
#define ARP_H

#include "../../mipd_common.h"
#include "../mip/mip.h"

#define ARP_TYPE_REQUEST 0
#define ARP_TYPE_RESPONSE 1

struct arp_message {
    u_int8_t type: 1;
    u_int8_t mip_addr: 8;
    u_int32_t padding: 23;
} __attribute__((packed));

int send_arp_request(int rsd, struct ifs_data ifs_data, u_int8_t dest_addr);

int handle_arp_packet(struct fds fds, struct ifs_data ifs_data, struct msghdr *msghdr);

#endif //ARP_H
