#ifndef ARP_QUEUE_H
#define ARP_QUEUE_H

#include "../mip.h"

int init_arp_queue(void);

int destroy_arp_queue(void);

int arp_enqueue_mip_pdu(struct mip_pdu const *pdu, u_int8_t next_hop);

struct mip_pdu *arp_dequeue_mip_pdu(u_int8_t mip);

struct mip_pdu *arp_peek_mip_pdu(u_int8_t mip);

#endif // ARP_QUEUE_H
