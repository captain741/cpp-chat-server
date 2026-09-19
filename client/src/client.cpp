#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

/*
-Gửi toàn bộ chuỗi dữ liệu qua socket
-Hàm trả về true nếu gửi thành công toàn bộ/ trả về false nếu socket bị lỗi hoặc đóng.
*/
namespace {
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

std::string encodeDraftLine(const std::string& line) {
    std::string encoded;
    for(char character : line) {
        if(character == '\\') {
            encoded += "\\\\";
        } else {
            encoded += character;
        } 
    }
    return encoded;
}

bool receiveLine(int socket_fd, std::string& line) {
    line.clear();

    while (true) {
        char character = '\0';

        const ssize_t bytes_received =
            recv(socket_fd, &character, 1, 0);

        if (bytes_received <= 0) {
            return false;
        }

        if (character == '\n') {
            return true;
        }

        line.push_back(character);
    }
}
}
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
std::string prompt;

if (!receiveLine(client_fd, prompt)) {
    perror("recv");
    close(client_fd);
    return 1;
}

std::cout << prompt << std::endl;

std::string username;

while (true) {
    if (!std::getline(std::cin, username)) {
        close(client_fd);
        return 1;
    }

    if (!sendAll(client_fd, username + "\n")) {
        perror("send");
        close(client_fd);
        return 1;
    }

    std::string response;

    if (!receiveLine(client_fd, response)) {
        perror("recv");
        close(client_fd);
        return 1;
    }

    if (response == "USERNAME_OK") {
        break;
    }

    if (response == "USERNAME_TAKEN") {
        std::cout
            << "Name already in use. Please enter another name."
            << std::endl;
        std::string retry_prompt;
        if(!receiveLine(client_fd, retry_prompt)) {
            perror("recv");
            close(client_fd);
            return 1;
        }
        std::cout << retry_prompt;
        continue;
    }

    std::cerr << "Unknown server response: "
              << response
              << std::endl;

    close(client_fd);
    return 1;
}
    
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
// // vòng lặp gửi tin nhắn 
//     std::string message;
//     while (std::getline(std::cin, message)) {
//         if (message.empty()) {
//             continue;
//         }

//         if (send(client_fd, message.c_str(), message.size(), 0) == -1) {
//             perror("send");
//             break;
//         }
//     }
// xử lí phần gửi tin nhắn 
std::string line;
std::string draft;
while (std::getline(std::cin, line)) {
    if(line == "/cancel") {
        draft.clear();
        std::cout << "Draft cancelled." << std::endl;
        continue;
    }
    if(line == ".") {
        if(draft.empty()) {
            std::cout << "Nothing to send." << std::endl;
            continue;
        }
        // them newline that de server biet frame da ket thuc
        const std::string frame = draft + "\n";

        if(!sendAll(client_fd, frame)) {
            perror("send");
            break;
        }
        draft.clear();
        continue;
    }
    if(!draft.empty()) {
        draft += "\\n";
    }
    draft += encodeDraftLine(line);
}

    close(client_fd);    // đóng socket 
    receiver.join();     // đợi thread receiver kết thúc rồi mới exit
    return 0;
}
