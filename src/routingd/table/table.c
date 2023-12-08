#include "table.h"
#include "../routing_common.h"
#include <stdlib.h>
#include <stdio.h>

// Global routing table
route_node *routing_table[MAX_NODES];

/**
 * Initializes the routing table for the MIP protocol.
 *
 * Sets each entry in the routing table to NULL, effectively clearing the table for new routing information.
 */
void init_routing_table() {
    for (int i = 0; i < MAX_NODES; i++) {
        routing_table[i] = NULL;
    }
}

/**
 * Adds a route to the routing table or updates an existing one.
 *
 * This function is responsible for adding a new route to the given destination
 * or updating an existing route via the specified next hop with the given cost.
 * If the route does not exist, it creates a new route node and adds it to the table.
 * If the route exists, it just updates the cost and sets its status to valid.
 *
 * @param dest: The MIP address of the destination node.
 * @param next_hop: The MIP address of the next hop node on the route to the destination.
 * @param cost: The cost of the route to the destination via the next hop node.
 */
void add_update_route(uint8_t dest, uint8_t next_hop, int cost) {
    route_node **current = &routing_table[dest];
    while (*current != NULL && (*current)->route.next_hop != next_hop) {
        current = &(*current)->next;
    }

    if (*current == NULL) { // Add new route
        *current = (route_node *)malloc(sizeof(route_node));
        (*current)->route.next_hop = next_hop;
        (*current)->route.cost = cost;
        (*current)->route.valid = 1;
        (*current)->next = NULL;
    } else { // Update existing route
        (*current)->route.cost = cost;
        (*current)->route.valid = 1;
    }

    //global_debug("Added/updated route to %d via %d with cost %d\n", dest, next_hop, cost);
    //print_routing_table();
}

/**
 * Checks if a specific route already exists in the routing table.
 *
 * This function determines if a route to a specific destination via a specified next hop
 * node already exists in the routing table.
 *
 * @param dest: The MIP address of the destination node.
 * @param next_hop: The MIP address of the next hop node on the route to the destination.
 * @return: 1 if the route exists, 0 otherwise.
 */
int route_exists(uint8_t dest, uint8_t next_hop) {
    route_node **current = &routing_table[dest];
    while (*current != NULL && (*current)->route.next_hop != next_hop) {
        current = &(*current)->next;
    }

    if (*current == NULL) {
        return 0;
    } else {
        return 1;
    }
}

/**
 * Deletes a specific route from the routing table.
 *
 * This function is responsible for finding and removing a specific route from the routing table.
 * The route is identified by the destination and the next hop node addresses. If the route exists,
 * the function removes it from the table and frees the allocated memory.
 *
 * @param dest: The MIP address of the destination node.
 * @param next_hop: The MIP address of the next hop node on the route to the destination.
 */
void delete_route(uint8_t dest, uint8_t next_hop) {
    route_node **current = &routing_table[dest];
    while (*current != NULL && (*current)->route.next_hop != next_hop) {
        current = &(*current)->next;
    }

    if (*current != NULL) {
        route_node *temp = *current;
        *current = (*current)->next;
        free(temp);
    }
}

/**
 * Marks a specific hop as unreachable in the routing table.
 *
 * This function iterates over the entire routing table and sets the cost to 255 (representing unreachable)
 * for any route that uses the specified next hop node. This function is typically called when the specified
 * next hop node has become unreachable, and hence all paths via that node should be updated to reflect this.
 *
 * @param next_hop: The MIP address of the next hop node that is now unreachable.
 */
void set_hop_unreachable(u_int8_t next_hop) {
    for (int i = 0; i < MAX_NODES; i++) {
        route_node *current = routing_table[i];
        while (current != NULL) {
            if (current->route.next_hop == next_hop) {
                current->route.cost = 255;
            }
            current = current->next;
        }
    }
}

/**
 * Finds the fastest route to a given destination in the routing table.
 *
 * This function iterates over all routes to the given destination in the routing table
 * and returns the one with the lowest cost. If no valid route is found, returns a route
 * with cost MAX_COST.
 *
 * @param dest: The MIP address of the destination node.
 * @return: A route_info structure containing information about the fastest route to the destination.
 */
route_info find_fastest_route(uint8_t dest) {
    route_node *current = routing_table[dest];
    route_info fastest_route = {0, MAX_COST, 0};

    while (current != NULL) {
        if (current->route.valid && current->route.cost < fastest_route.cost) {
            fastest_route = current->route;
        }
        current = current->next;
    }

    return fastest_route;
}

/**
 * Logs the entire routing table for debugging purposes.
 *
 * This function iterates over the entire routing table and logs each valid route including its destination,
 * next hop, and cost. Each line is formatted as "Destination Next Hop Cost".
 * This is mainly used for debugging and testing the correctness of the routing table implementation.
 */
void print_routing_table() {
    global_debug("Routing Table:\n");
    global_debug("%-12s %-12s %-12s\n", "Destination", "Next Hop", "Cost");

    for (int i = 0; i < MAX_NODES; i++) {
        route_node *current = routing_table[i];
        while (current != NULL) {
            if (current->route.valid) {
                global_debug("%-12d %-12d %-12d\n", i, current->route.next_hop, current->route.cost);
            }
            current = current->next;
        }
    }
}

/**
 * Computes the minimum cost routes for all reachable destinations considering a specific neighbor.
 *
 * This function calculates the minimum costs to all destinations with the exception of ones
 * where the given neighbor is the next hop. If a route with the neighbor as the next hop is found,
 * it'll be marked as MAX_COST as per the "poisoned reverse" technique.
 * This method helps avoid problems in routing algorithms known as count-to-infinity problem.
 *
 * @param dest_mip_addr: The MIP address of the neighbor node.
 * @param fastest_routes: Array to hold the cost of fastest route to every other node in the network.
 */
void get_all_fastest_routes_for_neighbour(uint8_t dest_mip_addr, u_int8_t fastest_routes[MAX_NODES]) {
    //global_debug("Getting all fastest routes for neighbour %d\n", dest_mip_addr);
    for (int node = 0; node < MAX_NODES; node++) {
        // Find the fastest route for each destination
        fastest_routes[node] = find_fastest_route(node).cost;

        // Apply Poisoned Reverse: mark route as invalid if the next hop is the neighbor
        if (find_fastest_route(node).next_hop == dest_mip_addr) {
            fastest_routes[node] = MAX_COST;
        }
    }
}

/**
 * Computes the minimum cost routes for all reachable destinations in the network.
 *
 * This function calculates the minimum costs to all destinations in the network by finding
 * the fastest route for each node. The results are stored in an array indexed by the node MIP address.
 *
 * @param fastest_routes: Array to hold the cost of fastest route to every other node in the network.
 */
void get_all_fastest_routes(uint8_t fastest_routes[MAX_NODES]) {
    //global_debug("Getting all fastest routes\n");
    for (int node = 0; node < MAX_NODES; node++) {
        // Find the fastest route for each destination
        fastest_routes[node] = find_fastest_route(node).cost;
    }
}

/**
 * Identifies all immediate neighbors.
 *
 * This function checks the entire routing table and identifies all nodes that are immediate neighbors.
 * A node is considered a neighbor if the cost of the fastest route to that node is 1. For each neighbor found,
 * it assigns 1 to the corresponding element in the neighbors array. For non-neighbors, it assigns 0.
 *
 * @param neighbours: Array to hold the identification of neighbour nodes. If node 'i' is a neighbour, neighbours[i] will be 1, 0 otherwise.
 */
void get_all_neighbours(uint8_t neighbours[MAX_NODES]) {
    // global_debug("Getting all neighbours\n");
    for (int node = 0; node < MAX_NODES; node++) {
        // Find the fastest route for each destination
        if (find_fastest_route(node).cost == 1) {
            //global_debug("Found neighbour %d\n", node);
            neighbours[node] = 1;
        } else {
            neighbours[node] = 0;
        }
    }
}


