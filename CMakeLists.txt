cmake_minimum_required(VERSION 3.26)
project(mipd C)

set(CMAKE_C_STANDARD 11)

add_executable(mipd main.c
        common.h
        common.c
        upper/upper.c
        upper/upper.h
        lower/util.c
        lower/util.h
        lower/mip/mip.c
        lower/mip/mip.h
        lower/mip/queue.c
        lower/mip/queue.h
        lower/arp/arp.c
        lower/arp/arp.h
        lower/arp/cache.c
        lower/arp/cache.h
)