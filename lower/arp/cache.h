#ifndef CACHE_H
#define CACHE_H


struct arp_cache_entry {
    u_int8_t mip_addr;
    u_int8_t mac_addr[6];
    u_int8_t interface;
};

struct arp_cache {
    struct arp_cache_entry entries[256];
    int size;
};

void arp_cache_init();

int arp_cache_add(u_int8_t mip_addr, u_int8_t mac_addr[6], u_int8_t interface);

int arp_cache_remove(u_int8_t mip_addr);

struct arp_cache_entry *arp_cache_get(u_int8_t mip_addr);

void arp_cache_print_to_debug();

void arp_cache_free();

#endif //CACHE_H