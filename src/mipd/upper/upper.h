#ifndef UPPER_H
#define UPPER_H

#include "../mipd_common.h"
#include "../lower/mip/mip.h"
#include <sys/socket.h>

struct unix_message {
    u_int8_t mip_addr;
    u_int8_t ttl;
    u_int8_t sdu[511];
};

int handle_usd_request(struct fds *fds);

int handle_usd_event(struct fds fds, struct ifs_data ifs_data, struct accepted_usd accepted_usd);

void send_ping_message(struct fds fds, struct mip_pdu mip_pdu);

int send_usd_message(int usd, struct mip_pdu mip_pdu);

#endif //UPPER_H
