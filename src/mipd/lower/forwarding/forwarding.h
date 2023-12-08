#ifndef FORWARDING_H
#define FORWARDING_H

#include "../../mipd_common.h"
#include "../mip/mip.h"
#include "../../upper/upper.h"

int forward_mip_pdu(struct fds fds, struct ifs_data ifs_data, struct mip_pdu mip_pdu, struct msghdr *msghdr);

#endif //FORWARDING_H
