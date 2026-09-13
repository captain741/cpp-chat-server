#include "ChatServer.h"

#include <iostream>

int main() {
    try {
        ChatServer server(8080, 10);    // tạo server trên port 8080 với số lượng đang chờ xử lí là 10 
        server.start();   // tạo socket, bind, listen, khởi động accept loop

        std::cout << "Press Ctrl+C to stop the server." << std::endl;
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line == "exit") {
                break;
            }
        }

        server.stop();   // dừng server 1 cách an toàn 
    } catch (const std::exception& e) {   // bắt exception từ socket/server
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
