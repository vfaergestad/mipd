# Makefile for mipd and routingd

CC = gcc
CFLAGS = -g
SRC_DIR = src
BUILD_DIR = .

# List of source files for mipd
MIPD_SRC = $(SRC_DIR)/mipd/main.c \
           $(SRC_DIR)/mipd/mipd_common.c \
           $(SRC_DIR)/mipd/upper/upper.c \
           $(SRC_DIR)/mipd/lower/lower.c \
           $(SRC_DIR)/mipd/lower/mip/mip.c \
           $(SRC_DIR)/mipd/lower/mip/queues/arp_queue.c \
           $(SRC_DIR)/mipd/lower/arp/arp.c \
           $(SRC_DIR)/mipd/lower/arp/cache.c \
           $(SRC_DIR)/mipd/lower/forwarding/forwarding.c \
           $(SRC_DIR)/mipd/lower/mip/queues/route_queue.c \
           $(SRC_DIR)/mipd/upper/routing/routing.c

# List of source files for routingd
ROUTINGD_SRC = $(SRC_DIR)/routingd/main.c \
			   $(SRC_DIR)/routingd/routing_common.c \
               $(SRC_DIR)/routingd/table/table.c \
               $(SRC_DIR)/routingd/hello/hello.c \
               $(SRC_DIR)/routingd/hello/checkin.c \
               $(SRC_DIR)/routingd/update/update.c \
               $(SRC_DIR)/routingd/request/request.c \
               $(SRC_DIR)/routingd/handle_messages.c

# Executables
MIPD_EXEC = mipd
ROUTINGD_EXEC = routingd
PING_CLIENT_EXEC = ping_client
PING_SERVER_EXEC = ping_server

all: $(MIPD_EXEC) $(ROUTINGD_EXEC) $(PING_CLIENT_EXEC) $(PING_SERVER_EXEC)

$(MIPD_EXEC): $(MIPD_SRC)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $^

$(ROUTINGD_EXEC): $(ROUTINGD_SRC)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $^

$(PING_CLIENT_EXEC): $(SRC_DIR)/ping_client/ping_client.c
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $^

$(PING_SERVER_EXEC): $(SRC_DIR)/ping_server/ping_server.c
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $^

clean:
	rm -rf $(BUILD_DIR)/$(MIPD_EXEC) $(BUILD_DIR)/$(ROUTINGD_EXEC) $(BUILD_DIR)/$(PING_CLIENT_EXEC) $(BUILD_DIR)/$(PING_SERVER_EXEC)

.PHONY: all clean
