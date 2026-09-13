#pragma once

#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

class ChatServer {
public:
    explicit ChatServer(uint16_t port = 8080, int backlog = 5);    // khởi tạo server với port mặc định 8080 và backlog là số lượng kết nối đang chờ xử lí 
    ~ChatServer();    // hàm hủy 
/*
Không cho copy objects
Vì server có socket, thread, mutex, tài nguyên độc quyền => nếu copy sẽ dễ lỗi
*/
    ChatServer(const ChatServer&) = delete;
    ChatServer& operator=(const ChatServer&) = delete;

    void start();     // khởi động server 
    void stop();      // dừng server an toàn 

private:
    void acceptLoop();    // hàm lặp vô hạn để accept client mới 
    void handleClient(int client_fd);      // xử lí client cụ thể 
    void broadcastMessage(const std::string& message, int sender_fd = -1);        // gửi message tới toàn bô client
    void removeClient(int client_fd);      // xóa client khỏi danh sách active
    std::string readUsername(int client_fd);       // Gửi prompt"Enter your name", nhận username từ client

    int server_fd_;     // file decriptor của socket server 
    uint16_t port_;     // Port lắng nghe 
    int backlog_;       // số lượng kết nối chờ xử lí 
    bool running_;      // trạng thái của server

    std::mutex clients_mutex_;        // khóa bảo vệ dữ liệu chung
    std::vector<int> clients_;        // danh sách các client đang online
    std::unordered_map<int, std::string> client_names_;      // mao socket fd -> username
    std::thread accept_thread_;             // thread đang chạy accept loop
};
