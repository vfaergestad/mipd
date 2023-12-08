#ifndef CHECKIN_H
#define CHECKIN_H

#include <sys/types.h>

void init_checkins();

void checkin_node(u_int8_t mip_addr);

int has_node_checked_in(u_int8_t mip_addr);

void uncheckin_node(u_int8_t mip_addr);



#endif //CHECKIN_H
