#ifndef HELLO_H
#define HELLO_H

#include "../routing_common.h"
#include "../../common/routing/routing_messages.h"

void handle_hello_message(int usd, hello_message message);

void send_hello_message(int usd);

#endif //HELLO_H
