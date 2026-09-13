#include "ChatServer.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

namespace {
std::string trimWhitespace(const std::string& value) {
    const std::string whitespace = " \t\r\n";
    const auto start = value.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }

    const auto end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}
}  // namespace

void ChatRoom::addClient(int client_fd, const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_[client_fd] = username;
}

void ChatRoom::removeClient(int client_fd) {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_.erase(client_fd);
}

void ChatRoom::broadcast(const std::string& message, int sender_fd) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [fd, username] : clients_) {
        (void)username;
        if (fd == sender_fd) {
            continue;
        }

        if (send(fd, message.c_str(), static_cast<int>(message.size()), 0) < 0) {
            perror("send");
        }
    }
}

std::vector<std::string> ChatRoom::getUsernames() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> usernames;
    usernames.reserve(clients_.size());
    for (const auto& [fd, username] : clients_) {
        (void)fd;
        usernames.push_back(username);
    }
    return usernames;
}

int ChatRoom::findClientByUsername(const std::string& username) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [fd, name] : clients_) {
        if (name == username) {
            return fd;
        }
    }
    return -1;
}

std::string ChatRoom::getUsername(int client_fd) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = clients_.find(client_fd);
    if (it == clients_.end()) {
        return "";
    }
    return it->second;
}

bool ChatRoom::contains(int client_fd) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return clients_.find(client_fd) != clients_.end();
}

ChatServer::ChatServer(uint16_t port, int backlog)
    : server_fd_(-1), port_(port), backlog_(backlog), running_(false) {}

ChatServer::~ChatServer() {
    stop();
}

void ChatServer::start() {
    if (running_) {
        return;
    }

    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        perror("socket");
        throw std::runtime_error("Failed to create server socket");
    }

    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_fd_);
        server_fd_ = -1;
        throw std::runtime_error("Failed to configure socket options");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd_);
        server_fd_ = -1;
        throw std::runtime_error("Failed to bind server socket");
    }

    if (listen(server_fd_, backlog_) == -1) {
        perror("listen");
        close(server_fd_);
        server_fd_ = -1;
        throw std::runtime_error("Failed to listen on socket");
    }

    running_ = true;
    accept_thread_ = std::thread(&ChatServer::acceptLoop, this);
    std::cout << "Chat server is listening on port " << port_ << std::endl;
}

void ChatServer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    if (server_fd_ != -1) {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
        server_fd_ = -1;
    }

    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
}

void ChatServer::acceptLoop() {
    while (running_) {
        int client_fd = accept(server_fd_, nullptr, nullptr);
        if (!running_) {
            break;
        }

        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        std::cout << "New client connected with fd = " << client_fd << std::endl;
        std::thread client_thread(&ChatServer::handleClient, this, client_fd);
        client_thread.detach();
    }
}

void ChatServer::broadcastMessage(const std::string& message, int sender_fd) {
    chat_room_.broadcast(message, sender_fd);
}

std::string ChatServer::readUsername(int client_fd) {
    const std::string prompt = "Enter your name: ";
    if (send(client_fd, prompt.c_str(), prompt.size(), 0) <= 0) {
        return "";
    }

    char name_buffer[128] = {0};
    const ssize_t bytes_read = recv(client_fd, name_buffer, sizeof(name_buffer) - 1, 0);
    if (bytes_read <= 0) {
        return "";
    }

    name_buffer[bytes_read] = '\0';
    return trimWhitespace(std::string(name_buffer));
}

bool ChatServer::handleCommand(int client_fd, const std::string& message, std::string& response) {
    if (message.empty() || message.front() != '/') {
        return false;
    }

    if (message == "/help") {
        response = "Available commands: /help, /who, /msg <username> <message>";
        return true;
    }

    if (message == "/who") {
        const auto users = chat_room_.getUsernames();
        if (users.empty()) {
            response = "No users online.";
            return true;
        }

        std::ostringstream oss;
        oss << "Online users: ";
        for (size_t i = 0; i < users.size(); ++i) {
            if (i > 0) {
                oss << ", ";
            }
            oss << users[i];
        }
        response = oss.str();
        return true;
    }

    if (message.rfind("/msg ", 0) == 0) {
        std::string payload = message.substr(5);
        const size_t space_pos = payload.find(' ');
        if (space_pos == std::string::npos) {
            response = "Usage: /msg <username> <message>";
            return true;
        }

        const std::string target_name = payload.substr(0, space_pos);
        const std::string target_message = payload.substr(space_pos + 1);
        const int target_fd = chat_room_.findClientByUsername(target_name);
        if (target_fd == -1) {
            response = "User not found.";
            return true;
        }

        const std::string sender_name = chat_room_.getUsername(client_fd);
        const std::string private_message = "[" + sender_name + " -> " + target_name + "]: " + target_message + "\n";
        send(target_fd, private_message.c_str(), private_message.size(), 0);

        if (client_fd != target_fd) {
            const std::string self_message = "[you -> " + target_name + "]: " + target_message + "\n";
            send(client_fd, self_message.c_str(), self_message.size(), 0);
        }

        response = "";
        return true;
    }

    response = "Unknown command. Try /help.";
    return true;
}

void ChatServer::handleClient(int client_fd) {
    std::string username = readUsername(client_fd);
    if (username.empty()) {
        close(client_fd);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        chat_room_.addClient(client_fd, username);
    }

    broadcastMessage(username + " joined the room\n");

    char buffer[1024] = {0};
    while (running_) {
        const ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received == 0) {
            std::cout << username << " disconnected" << std::endl;
            break;
        }

        if (bytes_received < 0) {
            perror("recv");
            break;
        }

        buffer[bytes_received] = '\0';
        std::string message(buffer);
        if (!message.empty() && message.back() == '\n') {
            message.pop_back();
        }

        std::string command_response;
        if (handleCommand(client_fd, message, command_response)) {
            if (!command_response.empty()) {
                send(client_fd, command_response.c_str(), command_response.size(), 0);
            }
            continue;
        }

        std::string formatted_message = std::string("[") + username + "]: " + message + "\n";
        std::cout << formatted_message;
        broadcastMessage(formatted_message, client_fd);
    }

    removeClient(client_fd);
    broadcastMessage(username + " left the room\n");
    close(client_fd);
}

void ChatServer::removeClient(int client_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    chat_room_.removeClient(client_fd);
}
