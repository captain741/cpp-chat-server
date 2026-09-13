#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

int main() {
    // địa chỉ server đang chạy
    const char* server_ip = "127.0.0.1";     // client kết nối với local host (127.0.0.1)
    const int server_port = 8080;

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);                  // tạo socket mới cho client
// nếu socket lỗi => dừng 
    if (client_fd == -1) {
        perror("socket");
        return 1;
    }
// khởi tạo địa chỉ server
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(client_fd);
        return 1;
    }
// kết nối client với server ( nếu kết nối không thành công in lỗi và đóng socket)
    if (connect(client_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        return 1;
    }

    std::cout << "Connected to chat server." << std::endl;
// Client nhận lời nhắn "Enter your name: " từ server
    char prompt[256] = {0};
    ssize_t prompt_size = recv(client_fd, prompt, sizeof(prompt) - 1, 0);     // đọc dữ liệu từ socket
    if (prompt_size > 0) {
        prompt[prompt_size] = '\0';
        std::cout << prompt;
    }

    std::string username;
    std::cout << "Your name: ";
    std::getline(std::cin, username);    // client nhập tên 
    send(client_fd, username.c_str(), username.size(), 0);   // Clinet gửi tên tới server => server lưu vào map
/*
*Thread nhận tin nhắn từ server:
- Đây là thread riêng để lắng nghe incoming mesages
- Trong khi main thread đang chờ user nhập tin nhắn , receiver thread đang nghe server trả về dữ liệu 
- Đây là mô hình chuẩn cho client chat
*/
    std::thread receiver([client_fd]() {
        char buffer[1024] = {0};
        while (true) {
            const ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0) {
                break;
            }
            buffer[bytes] = '\0';
            std::cout << buffer << std::flush;
        }
    });
// vòng lặp gửi tin nhắn 
    std::string message;
    while (std::getline(std::cin, message)) {
        if (message.empty()) {
            continue;
        }

        if (send(client_fd, message.c_str(), message.size(), 0) == -1) {
            perror("send");
            break;
        }
    }

    close(client_fd);    // đóng socket 
    receiver.join();     // đợi thread receiver kết thúc rồi mới exit
    return 0;
}
