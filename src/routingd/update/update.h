#ifndef UPDATE_H
#define UPDATE_H

#include "../../common/routing/routing_messages.h"
#include "../routing_common.h"

void send_update_messages(int usd);

void handle_update_message(int usd, const update_message message);

#endif //UPDATE_H
