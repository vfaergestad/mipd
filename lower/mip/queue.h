#ifndef QUEUE_H
#define QUEUE_H

#include "mip.h"

int init_queue(void);

int destroy_queue(void);

int enqueue_mip_pdu(struct mip_pdu *pdu);

struct mip_pdu *dequeue_mip_pdu(u_int8_t mip);

struct mip_pdu *peek_mip_pdu(u_int8_t mip);

#endif // QUEUE_H
