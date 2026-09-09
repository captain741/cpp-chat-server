#pragma once

#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

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

    int server_fd_;
    uint16_t port_;
    int backlog_;
    bool running_;

    std::mutex clients_mutex_;
    std::vector<int> clients_;
    std::unordered_map<int, std::string> client_names_;
    std::thread accept_thread_;
};
