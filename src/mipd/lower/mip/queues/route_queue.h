#ifndef ROUTE_QUEUE_H
#define ROUTE_QUEUE_H

#include <stdlib.h>
#include "../mip.h"

struct queue_node {
    struct mip_pdu pdu;
    struct queue_node *next;
};

struct route_queue {
    struct queue_node *front;
    struct queue_node *rear;
    size_t size;
};

static struct route_queue route_queue;

void init_route_queue();

int route_enqueue(struct mip_pdu pdu);

struct mip_pdu route_dequeue();

int is_route_queue_empty();

void free_route_queue();

#endif // ROUTE_QUEUE_H
