#ifndef TABLE_H
#define TABLE_H

#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>

#define MAX_NODES 256
#define MAX_COST 255

// Structure for each route info
typedef struct {
    uint8_t next_hop;
    u_int8_t cost;
    int valid;
} route_info;

// Node in the linked list for each destination
typedef struct route_node {
    route_info route;
    struct route_node *next;
} route_node;

void init_routing_table();

void add_update_route(uint8_t dest, uint8_t next_hop, int cost);

int route_exists(uint8_t dest, uint8_t next_hop);

void delete_route(uint8_t dest, uint8_t next_hop);

void set_hop_unreachable(u_int8_t next_hop);

route_info find_fastest_route(uint8_t dest);

void print_routing_table();

void get_all_fastest_routes_for_neighbour(uint8_t dest_mip_addr, u_int8_t fastest_routes[MAX_NODES]);

void get_all_fastest_routes(uint8_t fastest_routes[MAX_NODES]);

void get_all_neighbours(uint8_t neighbours[MAX_NODES]);

#endif // TABLE_H
