#include "ChatServer.h"
#include <cctype>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

namespace {
    constexpr std::size_t kMaxFrameSize = 64 * 1024;
        bool sendAll(int socket_fd, const std::string& data) {
        std::size_t total_sent = 0;     // số byte đã gửi thành công 

        while (total_sent < data.size()) {     // tiếp tục cho đến khi toàn bộ data dc gửi
            const ssize_t bytes_sent = send(
                socket_fd,                       // file descreptor cuar socket
                data.data() + total_sent,       // vị trí du lieu chua gui
                data.size() - total_sent,       // so byte con lai
                MSG_NOSIGNAL                    // tranh SIGIPE khi socket da dong
            );

            // send() tra ve so byte da gui
            // Gia tri <= 0 nghia la co loi or ket noi da dong
            if(bytes_sent <= 0) {
                return false;
            }
            // cong so byte vua gui vao tong so byte da gui.
            total_sent += static_cast<std::size_t>(bytes_sent);;
        }
        return true;     // toan bo chuoi dc gui thanh cong
    }

/*
- Chuoi ket qua sau khi giai ma
- Day la message that ma server se broadcast
*/
std::string decodeMessage(const std::string& encoded_message) {
    std::string message;
    for(std::size_t index = 0; index < encoded_message.size(); ++index) {
        if (encoded_message[index] == '\\' && index + 1 < encoded_message.size()) {
            const char escaped_character = encoded_message[index + 1];
            if(escaped_character =='n') {
                message.push_back('\n');
                ++index;
                continue;
            }
            if(escaped_character == '\\') {
                message.push_back('\\');
                ++index;
                continue;
            }
        }
        message.push_back(encoded_message[index]);
    }
    return message;
}

std::string normalizeUsername(const std::string& username) {
    std::string normalized = username;
    for(char& character : normalized) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return normalized;
}

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

bool ChatRoom::addClient(int client_fd, const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [existing_fd, existing_username] : clients_) {
        (void)existing_fd;
        if (normalizeUsername(existing_username) == normalizeUsername(username)) {    // kiem tra username trung ten
            return false;
        }
    }

    clients_[client_fd] = username;
    return true;
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

        // if (send(fd, message.c_str(), static_cast<int>(message.size()), 0) < 0) {
        //     perror("send");
        // }
        // giai quyet broadcast message dai khong bi gui thieu
        if(!sendAll(fd, message)) {
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
        if (normalizeUsername(name) == normalizeUsername(username)) {
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
    const std::string prompt = "Enter your name:\n";
    if(!sendAll(client_fd, prompt)) {
        return "";
    }
    std::string username;
    while (username.size() < 128) {
        char character = '\0';
        const ssize_t bytes_read = recv(client_fd, &character, 1, 0);
        if (bytes_read <= 0) {
            return "";
        }
        if (character == '\n') {
            break;
        }
        username.push_back(character);
    }
    return trimWhitespace(username);
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
//        send(target_fd, private_message.c_str(), private_message.size(), 0);
        if (!sendAll(target_fd, private_message)) {
            perror("send");
        }
        if (client_fd != target_fd) {
            const std::string self_message = "[you -> " + target_name + "]: " + target_message + "\n";
//            send(client_fd, self_message.c_str(), self_message.size(), 0);
          if(!sendAll(client_fd, self_message)) {
            perror("send");
          }
}

        response = "";
        return true;
    }

    response = "Unknown command. Try /help.";
    return true;
}

void ChatServer::handleClient(int client_fd) {
    std::string username;
    while(true) {
        username = readUsername(client_fd);
        if(username.empty()) {
            close(client_fd);
            return;
        }
        if(chat_room_.addClient(client_fd, username)) {
            // thong bao cho client rang usernam dc chap nhan
            if(!sendAll(client_fd, "USERNAME_OK\n")) {
                perror("send");
                close(client_fd);
                return; 
            }
            break;
        }
        // neu username bi trung , cho phep client nhap ten khac
        if(!sendAll(client_fd, "USERNAME_TAKEN\n")) {
            perror("send");
            close(client_fd);
            return;
        }
    }

    broadcastMessage(username + " joined the room\n");

    char buffer[4096] = {0};
    std::string pending_data;
    while (running_) {
        const ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) , 0);
        if (bytes_received == 0) {
            std::cout << username << " disconnected" << std::endl;
            break;
        }

        if (bytes_received < 0) {
            perror("recv");
            break;
        }

     pending_data.append(buffer,static_cast<std::size_t>(bytes_received));
     if(pending_data.size() > kMaxFrameSize) {
        std::cerr << "Frame from " << username << "is too large" << std::endl;
        break;
     }
     std::size_t delimiter_position = pending_data.find('\n');
     while (delimiter_position != std::string::npos) {
        const std::string encoded_message = pending_data.substr(0,delimiter_position);
        pending_data.erase(0,delimiter_position + 1);
        const std::string message = decodeMessage(encoded_message);
        std::string command_response;
        if(handleCommand(client_fd, message,command_response)) {
            if(!command_response.empty()) {
                const std::string response = command_response + "\n";
                if(!sendAll(client_fd, response)) {
                    perror("send");
                    break;
                }
            }
        } else if (!message.empty()) {
            const std::string formatted_message = "[" + username + "]: " + message + "\n";
            std::cout << formatted_message ;
            broadcastMessage(formatted_message, client_fd);
        }
        delimiter_position = pending_data.find('\n');
     }
}

    removeClient(client_fd);
    broadcastMessage(username + " left the room\n");
    close(client_fd);
}

void ChatServer::removeClient(int client_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    chat_room_.removeClient(client_fd);
}
