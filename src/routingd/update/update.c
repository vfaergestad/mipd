#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include "update.h"

/**
 * Sends update message to a neighbor node.
 *
 * This function is used to inform one of the neighbor nodes about the current state of the routing table.
 * The function packs the routing information including the fastest routes to all nodes into an update_message
 * and subsequently sends it to the neighbor specified by dest_mip_addr.
 *
 * @param usd: The Unix domain socket descriptor used for communicating between MIP daemons.
 * @param dest_mip_addr: The MIP address of the neighbor node to send the update to.
 * @param fastest_routes: Array containing the minimum cost to every other node in the network.
 */
void send_update_message(int usd, u_int8_t dest_mip_addr, const u_int8_t fastest_routes[MAX_NODES]) {
    global_debug("Sending UPDATE message to %d", dest_mip_addr);
    update_message update;
    update.header.mip_addr = dest_mip_addr;
    update.header.ttl = 1;
    update.header.id1 = 0x55; // U
    update.header.id2 = 0x50; // P
    update.header.id3 = 0x44; // D
    memcpy(update.fastest_routes, fastest_routes, sizeof(u_int8_t) * MAX_NODES);
    //global_debug("Fastest routes to send: ");
    //for (int i = 0; i < 20; i++) {
    //    global_debug("%d,", update.fastest_routes[i]);
    //}


    if (send(usd, &update, sizeof(update), 0) == -1) {
        perror("send");
    }
}

/**
 * Sends the current state of the routing table to all neighbor nodes.
 *
 * This function is used to inform all neighbor nodes about the current state of the node's routing table.
 * First, it identifies all neighbors. Then for each neighbor, it calculates the minimum cost routes considering the
 * poisoned reverse rule and sends this data as an update_message to the neighbor node.
 *
 * @param usd: The Unix domain socket descriptor used for communicating between MIP daemons.
 */
void send_update_messages(int usd) {
    //global_debug("Sending UPDATE messages\n");
    u_int8_t neighbours[MAX_NODES];
    get_all_neighbours(neighbours);

    for (int node = 0; node < MAX_NODES; node++) {
        if (neighbours[node] != 0) {
            //global_debug("Neighbour: %d\n", node);
            u_int8_t fastest_routes[MAX_NODES];
            //global_debug("Getting the fastest routes to send to neighbour %d\n", node);
            get_all_fastest_routes_for_neighbour(node, fastest_routes);
            //global_debug("Sending UPDATE message to neighbour %d\n", node);
            //global_debug("Fastest routes to send: ");
            //for (int i = 0; i < 20; i++) {
            //    global_debug("%d,", fastest_routes[i]);
            //}
            send_update_message(usd, node, fastest_routes);
        }
    }
}

/**
 * Handles the received UPDATE message and updates the routing table accordingly.
 *
 * This function first checks whether the sender is already recognized as a neighbor or not. If not, it adds
 * the sender as a new neighbor. It then updates the routing table based on the received data.
 * If any changes have been made to the routing table, the function sends an updated routing table to all neighbors.
 * If no changes were made, it only prints the current state of the routing table.
 *
 * @param usd: The Unix domain socket descriptor used for communicating between MIP daemons.
 * @param message: The received UPDATE message from a neighbor node.
 */
void handle_update_message(int usd, const update_message message) {
    uint8_t sender_mip_addr = message.header.mip_addr;
    global_debug("Received UPDATE message from %d\n", sender_mip_addr);

    // Get all current fastest routes
    u_int8_t current_fastest_routes[MAX_NODES];
    get_all_fastest_routes(current_fastest_routes);
    //global_debug("Current fastest routes: ");
    // for (int i = 0; i < 20; i++) {
    //     global_debug("%d,", current_fastest_routes[i]);
    // }

    u_int8_t received_routes[MAX_NODES];
    //global_debug("Received routes: ");
    //for (int i = 0; i < 20; i++) {
    //    global_debug("%d,", message.fastest_routes[i]);
    //}
    memcpy(received_routes, message.fastest_routes, sizeof(u_int8_t) * MAX_NODES);
    //global_debug("Copied routes: ");
    //for (int i = 0; i < 20; i++) {
    //    global_debug("%d,", received_routes[i]);
    //}



    int fastest_route_changed = 0;
    // Get current fastest route to 'node'
    u_int8_t neighbour_cost = find_fastest_route(sender_mip_addr).cost;
    // Check if we have a fastest route of 1 to the sender, if not, it's a new neighbour.
    if (neighbour_cost != 1) {
        // This is a new neighbour. Add a route to the sender with a cost of 1
        add_update_route(sender_mip_addr, sender_mip_addr, 1);
        neighbour_cost = 1;
        // This was a new fastest route, so send an UPDATE message later
        fastest_route_changed = 1;
        global_debug("Added %d as a new neighbour\n", sender_mip_addr);
    }


    for (int node = 0; node < MAX_NODES; node++) {
        if (node != sender_mip_addr) { // If not the sender
            if (received_routes[node] == MAX_COST) { // If the received route is unreachable
                if (route_exists(node, sender_mip_addr)) { // And we have a route to the node via the sender
                    // This route is unreachable
                    delete_route(node, sender_mip_addr);
                    fastest_route_changed = 1;
                    continue;
                }
            } else { // If the received route is reachable
                add_update_route(node, sender_mip_addr, received_routes[node] + neighbour_cost);
            }

        }
    }

    // Check if our fastest routes have changed
    for (int node = 0; node < MAX_NODES; node++) {
        if (current_fastest_routes[node] != find_fastest_route(node).cost) {
            global_debug("Fastest route to %d\n has changed", node);
            fastest_route_changed = 1;
            print_routing_table();
            break;
        }
    }

    if (fastest_route_changed) {
        send_update_messages(usd);
    } else {
        global_debug("No new fastest route found, not sending UPDATE messages\n");
        print_routing_table();
    }
}