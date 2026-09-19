#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

class ChatRoom {
public:
    bool addClient(int client_fd, const std::string& username);
    void removeClient(int client_fd);
    void broadcast(const std::string& message, int sender_fd = -1) const;

    std::vector<std::string> getUsernames() const;
    int findClientByUsername(const std::string& username) const;
    std::string getUsername(int client_fd) const;
    bool contains(int client_fd) const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<int, std::string> clients_;
};

class ChatServer {
public:
    explicit ChatServer(uint16_t port = 8080, int backlog = 5);
    ~ChatServer();

    ChatServer(const ChatServer&) = delete;
    ChatServer& operator=(const ChatServer&) = delete;

    void start();
    void stop();

private:
    void acceptLoop();
    void handleClient(int client_fd);
    void broadcastMessage(const std::string& message, int sender_fd = -1);
    void removeClient(int client_fd);
    std::string readUsername(int client_fd);
    bool handleCommand(int client_fd, const std::string& message, std::string& response);

    int server_fd_;
    uint16_t port_;
    int backlog_;
    bool running_;

    std::mutex clients_mutex_;
    ChatRoom chat_room_;
    std::thread accept_thread_;
};
