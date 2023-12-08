#include "checkin.h"
#include "../table/table.h"

u_int8_t checkins[MAX_NODES];

/**
 * Initializes the check-ins table for MIP nodes.
 *
 * Sets all entries in the check-ins table to zero, indicating no nodes have checked in yet.
 */
void init_checkins() {
    for (int i = 0; i < MAX_NODES; i++) {
        checkins[i] = 0;
    }
}

/**
 * Marks the node identified by the given MIP address as having checked in.
 *
 * @param mip_addr: The MIP address of the node which has checked in.
 */
void checkin_node(u_int8_t mip_addr) {
    checkins[mip_addr] = 1;
}

/**
 * Checks if a node, identified by the given MIP address, has checked in.
 *
 * @param mip_addr: The MIP address of the node to check.
 * @return: The check-in status of the node identified by mip_addr, 1 for checked in, 0 otherwise.
 */
int has_node_checked_in(u_int8_t mip_addr) {
    return checkins[mip_addr];
}

/**
 * Removes the check-in status of the node identified by the given MIP address.
 *
 * @param mip_addr: The MIP address of the node to uncheck.
 */
void uncheckin_node(u_int8_t mip_addr) {
    checkins[mip_addr] = 0;
}