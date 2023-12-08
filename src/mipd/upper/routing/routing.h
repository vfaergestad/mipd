#ifndef ROUTING_H
#define ROUTING_H

#include <sys/types.h>
#include "../../mipd_common.h"
#include "../upper.h"

void forward_routing_message(struct fds fds, struct mip_pdu mip_pdu);

int send_routing_request(int usd, struct ifs_data ifs_data, u_int8_t dest_addr);

#endif //ROUTING_H
