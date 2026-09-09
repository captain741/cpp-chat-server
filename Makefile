CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pthread -Iserver/include
BIN_DIR := bin
SERVER_BIN := $(BIN_DIR)/chat_server
CLIENT_BIN := $(BIN_DIR)/chat_client

all: $(SERVER_BIN) $(CLIENT_BIN)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(SERVER_BIN): server/src/main.cpp server/src/ChatServer.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) server/src/main.cpp server/src/ChatServer.cpp -o $(SERVER_BIN)

$(CLIENT_BIN): client/src/client.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) client/src/client.cpp -o $(CLIENT_BIN)

run-server: $(SERVER_BIN)
	./$(SERVER_BIN)

run-client: $(CLIENT_BIN)
	./$(CLIENT_BIN)

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean run-server run-client
