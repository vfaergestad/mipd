#ifndef UPPER_H
#define UPPER_H

#include "../common.h"
#include "../lower/mip/mip.h"
#include <sys/socket.h>

int handle_usd_event(struct fds fds, struct ifs_data ifs_data);

int handle_ping(int usd, struct ifs_data ifs_data, struct msghdr msghdr);
#endif //UPPER_H
