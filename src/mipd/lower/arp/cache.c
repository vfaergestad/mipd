#include <stdlib.h>
#include <string.h>
#include "../../mipd_common.h"
#include "cache.h"

struct arp_cache *arp_cache;

/**
 * Initializes the mip_arp_cache. The max size of the cache is set in arp.h.
 */
void arp_cache_init() {
    global_debug("Initializing ARP cache");
    arp_cache = malloc(sizeof(struct arp_cache));
    arp_cache->size = 0;
}

/**
 * Takes the given values and creates and adds a entry to the mip_arp_cache.
 * @param mip_addr 8-bit MIP address that maps to the MAC address. Type: u_int_8.
 * @param mac_addr 48-bit MAC address that maps to the MIP address. Type: u_int8_t array[6].
 * @param interface The interface on which the response was received. Type: u_int8_t.
 * @return 0 if no errors. -1 if cache is full.
 */
int arp_cache_add(u_int8_t mip_addr, u_int8_t const mac_addr[6], u_int8_t interface){
    //global_debug("Adding entry: %d -> %02x:%02x:%02x:%02x:%02x:%02x to ARP cache", mip_addr,
    //             mac_addr[0],
    //            mac_addr[1],
    //             mac_addr[2],
    //             mac_addr[3],
    //             mac_addr[4],
    //             mac_addr[5]);
    if (arp_cache->size >= 256) {
        global_debug("ARP cache is full");
        return -1;
    }

    struct arp_cache_entry new_entry;
    new_entry.mip_addr = mip_addr;
    memcpy(new_entry.mac_addr, mac_addr, 6);
    new_entry.interface = interface;

    arp_cache->entries[arp_cache->size] = new_entry;
    arp_cache->size++;
    return 0;
}

/**
 * Removes the entry with the given MIP address from the ARP cache.
 * @param mip_addr The MIP address of the entry to remove. Type: u_int8_t.
 * @return 0 if no errors. -1 if entry was not found.
 */
int arp_cache_remove(u_int8_t mip_addr) {
    //global_debug("Removing entry: %d from ARP cache", mip_addr);
    for (int i = 0; i < arp_cache->size; i++) {
        if (arp_cache->entries[i].mip_addr == mip_addr) {
            // Move the last entry the position of the entry to be removed
            arp_cache->entries[i] = arp_cache->entries[arp_cache->size - 1];
            // Decrement the size of the cache
            arp_cache->size--;
            return 0;
        }
    }
    global_debug("Entry: %s was not found in ARP cache", mip_addr);
    return -1;  // Entry was not found
}

/**
 * Returns the MAC address of the entry with the given MIP address.
 * @param mip_addr The MIP address of the entry to get. Type: u_int8_t.
 * @return The MAC address of the entry. Type: u_int8_t array[6].
 */
struct arp_cache_entry *arp_cache_get(u_int8_t mip_addr) {
    //global_debug("Getting entry: %d from ARP cache", mip_addr);
    for (int i = 0; i < arp_cache->size; i++) {
        if (arp_cache->entries[i].mip_addr == mip_addr) {
            return &arp_cache->entries[i];
        }
    }
    global_debug("Entry: %d was not found in ARP cache", mip_addr);
    return NULL;  // Entry was not found
}

/**
 * Prints the ARP cache to stdout using global_debug.
 */
void arp_cache_print_to_debug() {
    global_debug("Printing ARP cache");
    for (int i = 0; i < arp_cache->size; i++) {
        global_debug("Entry %d: %d -> %02x:%02x:%02x:%02x:%02x:%02x",
                     i,
                     arp_cache->entries[i].mip_addr,
                     arp_cache->entries[i].mac_addr[0],
                     arp_cache->entries[i].mac_addr[1],
                     arp_cache->entries[i].mac_addr[2],
                     arp_cache->entries[i].mac_addr[3],
                     arp_cache->entries[i].mac_addr[4],
                     arp_cache->entries[i].mac_addr[5]);
    }
}