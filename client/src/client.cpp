#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

int main() {
    const char* server_ip = "127.0.0.1";
    const int server_port = 8080;

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(client_fd);
        return 1;
    }

    if (connect(client_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        return 1;
    }

    std::cout << "Connected to chat server." << std::endl;

    char prompt[256] = {0};
    ssize_t prompt_size = recv(client_fd, prompt, sizeof(prompt) - 1, 0);
    if (prompt_size > 0) {
        prompt[prompt_size] = '\0';
        std::cout << prompt;
    }

    std::string username;
    std::cout << "Your name: ";
    std::getline(std::cin, username);
    send(client_fd, username.c_str(), username.size(), 0);

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

    close(client_fd);
    receiver.join();
    return 0;
}
