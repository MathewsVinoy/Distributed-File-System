CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -g

# Output directory
BUILD_DIR = build

# Output binaries
SERVER = $(BUILD_DIR)/ser
METASERVER = $(BUILD_DIR)/metaser
CLIENT = $(BUILD_DIR)/cli

# Source files (actual paths in the repo)
SERVER_SRC = src/sever.c
METASERVER_SRC = src/metadata_server.c
METADATA_SRC = src/metadata_res/load_metadata.c
CLIENT_SRC = src/client.c src/clint_res/connction_cash.c

# Default target
all: $(BUILD_DIR) $(SERVER) $(METASERVER) $(CLIENT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER)

$(METASERVER): $(METASERVER_SRC) $(METADATA_SRC)
	$(CC) $(CFLAGS) $(METASERVER_SRC) $(METADATA_SRC) -o $(METASERVER)

$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT)

clean:
	rm -rf $(BUILD_DIR)


.PHONY: all clean


