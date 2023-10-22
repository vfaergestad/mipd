#ifndef LOWER_H
#define LOWER_H

#include <stdint.h>
#include <unistd.h>
#include <linux/if_packet.h>
#include <sys/socket.h>
#include "../../common.h"

#define MIP_BROADCAST_ADDR      255
#define MIP_BROADCAST_MAC_ADDR  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
#define MIP_BROADCAST_TTL       1
#define MIP_MAX_TTL             15
#define MIP_SDU_TYPE_ARP        0x01
#define MIP_SDU_TYPE_PING       0x02

struct mip_pdu {
    u_int8_t dest_addr: 8;
    u_int8_t src_addr: 8;
    u_int8_t ttl: 4;
    u_int16_t sdu_len: 9;
    u_int8_t sdu_type: 3;
    u_int8_t sdu[256];
} __attribute__((packed));

struct ether_frame {
    uint8_t dst_addr[6];
    uint8_t src_addr[6];
    uint8_t eth_proto[2];
} __attribute__((packed));

int send_mip_packet(int rsd, struct ifs_data ifs_data, struct mip_pdu mip_pdu);

int send_broadcast_packet(int rsd, struct ifs_data ifs_data, u_int8_t *sdu, int sdu_len);

int handle_mip_packet(int rsd, int usd, struct msghdr msghdr, struct ifs_data ifs_data);

int check_mip_queue(int rsd, u_int8_t mip_addr, struct ifs_data ifs_data);

#endif //LOWER_H