#include "ChatServer.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
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
// khởi tạo cấu trúc địa chỉ server
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    server_addr.sin_addr.s_addr = INADDR_ANY;
// gắn socket với 1 port cụ thể 
    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd_);
        server_fd_ = -1;
        throw std::runtime_error("Failed to bind server socket");
    }
// chuyển socket sang trạng thái lắng nghe 
    if (listen(server_fd_, backlog_) == -1) {
        perror("listen");
        close(server_fd_);
        server_fd_ = -1;
        throw std::runtime_error("Failed to listen on socket");
    }
// server bắt đầu lắng nghe kết nối mới trong nền
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
// gửi message đến tất cả client ngoại trừ sender
void ChatServer::broadcastMessage(const std::string& message, int sender_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (int fd : clients_) {
        if (fd == sender_fd) {
            continue;
        }

        if (send(fd, message.c_str(), static_cast<int>(message.size()), 0) < 0) {
            perror("send");
        }
    }
}
// server gửi yêu cầu nhập tên 
std::string ChatServer::readUsername(int client_fd) {
    const std::string prompt = "Enter your name: ";
    if (send(client_fd, prompt.c_str(), prompt.size(), 0) <= 0) {
        return "";
    }
// nhận username từ client qua socket
    char name_buffer[128] = {0};
    const ssize_t bytes_read = recv(client_fd, name_buffer, sizeof(name_buffer) - 1, 0);
    if (bytes_read <= 0) {     // nếu client disconnect hoặc lỗi trả về rỗng 
        return "";
    }
// cắt kí tự NULL và khoảng trắng rồi trả về đúng user
    name_buffer[bytes_read] = '\0';
    return trimWhitespace(std::string(name_buffer));
}

void ChatServer::handleClient(int client_fd) {
    std::string username = readUsername(client_fd);
    if (username.empty()) {    // nếu username rỗng hoặc không hợp lệ thì đóng socket và bỏ qua 
        close(client_fd);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.push_back(client_fd);       // thêm client vào danh sách online
        client_names_[client_fd] = username;    // lưu mapping fd-> username
    }

    broadcastMessage(username + " joined the room\n");    // thông báo cho các client khác có người vào phòng 
// tiếp tục nhận dũ liệu từ client cho đến khi disconnect hoặc server stop
    char buffer[1024] = {0};
    while (running_) {
        const ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received == 0) {         // recv() == 0 nghĩa là server đóng kết nối
            std::cout << username << " disconnected" << std::endl;
            break;
        }
// nếu lỗi network -> dừng xứ lí client đó 
        if (bytes_received < 0) {
            perror("recv");
            break;
        }

        buffer[bytes_received] = '\0';
        std::string message = std::string("[") + username + "]: " + buffer;    // tạo message dạng : [Steven]: hello
// chỉnh sửa lại định dạng để dễ hiển thị 
        if (!message.empty() && message.back() == '\n') {
            message.pop_back();
        }
        message += "\n";

        std::cout << message;
        broadcastMessage(message, client_fd);     // gửi mesage tới các client còn lại
    }

    removeClient(client_fd);     // xóa client 
    broadcastMessage(username + " left the room\n");    // thông báo người rời 
    close(client_fd);         // đóng socket
}
// hàm xóa client 
void ChatServer::removeClient(int client_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    clients_.erase(std::remove(clients_.begin(), clients_.end(), client_fd), clients_.end());
    client_names_.erase(client_fd);
}
