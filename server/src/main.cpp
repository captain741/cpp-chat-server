#include "ChatServer.h"

#include <iostream>

int main() {
    try {
        ChatServer server(8080, 10);
        server.start();

        std::cout << "Press Ctrl+C to stop the server." << std::endl;
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line == "exit") {
                break;
            }
        }

        server.stop();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
