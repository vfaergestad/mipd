cmake_minimum_required(VERSION 3.26)
project(src/mipd C)

set(CMAKE_C_STANDARD 11)

add_executable(src/mipd src/mipd/main.c
        src/mipd/mipd_common.h
        src/mipd/mipd_common.c
        src/mipd/upper/upper.c
        src/mipd/upper/upper.h
        src/mipd/lower/lower.c
        src/mipd/lower/lower.h
        src/mipd/lower/mip/mip.c
        src/mipd/lower/mip/mip.h
        src/mipd/lower/mip/queues/arp_queue.c
        src/mipd/lower/mip/queues/arp_queue.h
        src/mipd/lower/arp/arp.c
        src/mipd/lower/arp/arp.h
        src/mipd/lower/arp/cache.c
        src/mipd/lower/arp/cache.h
        src/mipd/lower/forwarding/forwarding.c
        src/mipd/lower/forwarding/forwarding.h
        src/mipd/lower/mip/queues/route_queue.c
        src/mipd/lower/mip/queues/route_queue.h
        src/mipd/upper/routing/routing.c
        src/mipd/upper/routing/routing.h
)

add_executable(src/routingd src/routingd/main.c
        src/routingd/table/table.c
        src/routingd/table/table.h
        src/routingd/hello/hello.c
        src/routingd/hello/hello.h
        src/common/routing/routing_messages.h
        src/routingd/update/update.c
        src/routingd/update/update.h
        src/routingd/request/request.c
        src/routingd/request/request.h
        src/routingd/handle_messages.c
        src/routingd/handle_messages.h
        src/routingd/routing_common.c
        src/routingd/routing_common.h
        src/routingd/hello/checkin.c
        src/routingd/hello/checkin.h
)

add_executable(src/ping_client src/ping_client/ping_client.c)

add_executable(src/ping_server src/ping_server/ping_server.c)