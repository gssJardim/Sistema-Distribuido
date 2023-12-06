

BIN_DIR = binary
OBJ_DIR = object
INC_DIR = include
SRC_DIR = source
PRO_DIR = protobuf
LIB_DIR = lib

CC = gcc
PROTOC = protoc

CFLAGS = -I $(INC_DIR) -pthread
PROTOC_FLAGS = -I /usr/local/include -L /usr/local/lib -lprotobuf-c

# Define the objects to be compiled
MAIN_OBJS = $(addprefix $(OBJ_DIR)/, tree_client.o tree_server.o)
CLIENT_LIB_OBJS = $(addprefix $(OBJ_DIR)/, data.o entry.o message.o shared.o client_stub.o network_client.o sdmessage.pb-c.o)
SERVER_LIB_OBJS = $(addprefix $(OBJ_DIR)/, data.o entry.o tree.o message.o tree_skel.o network_server.o shared.o sdmessage.pb-c.o)
LIB_OBJS = $(addprefix $(LIB_DIR)/, client-lib.o server-lib.o)

all: compile_protobuf tree_server tree_client

tree_client: $(MAIN_OBJS) $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/tree-client $(OBJ_DIR)/tree_client.o $(LIB_DIR)/client-lib.o $(PROTOC_FLAGS)

tree_server: $(MAIN_OBJS) $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/tree-server $(OBJ_DIR)/tree_server.o $(LIB_DIR)/server-lib.o $(PROTOC_FLAGS)

compile_protobuf:
	$(PROTOC) -I=$(PRO_DIR) --c_out=. sdmessage.proto
	mv sdmessage.pb-c.h $(INC_DIR)
	mv sdmessage.pb-c.c $(SRC_DIR)

$(LIB_DIR)/client-lib.o: $(CLIENT_LIB_OBJS)
	ld -r -o $@ $(CLIENT_LIB_OBJS)

$(LIB_DIR)/server-lib.o: $(SERVER_LIB_OBJS)
	ld -r -o $@ $(SERVER_LIB_OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(BIN_DIR)/*
	rm -f $(OBJ_DIR)/*
	rm -f $(LIB_DIR)/*
	rm -f $(INC_DIR)/sdmessage.pb-c.h
	rm -f $(SRC_DIR)/sdmessage.pb-c.c

