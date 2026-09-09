# Mentor roadmap for the C++ chat server project

## Mục tiêu

Project này là một mini-system socket + multithreading để bạn hiểu rõ các khái niệm cốt lõi của C++ backend:

- Linux socket programming
- TCP/IP
- concurrency và race condition
- mutex và synchronization
- OOP + resource ownership
- build system và project structure
- interview-oriented explanation

---

## Bước 1: Hiểu bài toán

Hãy tự trả lời các câu hỏi sau:

1. Server cần làm gì?
2. Client cần làm gì?
3. 1 server có thể xử lý nhiều client như thế nào?
4. Khi client gửi tin nhắn, server phải làm gì?
5. Khi client disconnect, server phải cleanup như thế nào?

Mục tiêu: bạn không chỉ viết code, mà phải hiểu luồng dữ liệu thực tế.

---

## Bước 2: Nắm rõ vòng đời socket

Dòng logic chuẩn của TCP server:

1. socket()
2. setsockopt()
3. bind()
4. listen()
5. accept()
6. recv()/send()
7. close()

Bạn nên giải thích bằng lời từng hàm:

- socket(): tạo socket endpoint
- bind(): gắn socket vào port cụ thể
- listen(): chuyển state sang listening
- accept(): chấp nhận một kết nối mới
- recv(): nhận dữ liệu từ client
- send(): gửi dữ liệu tới client

---

## Bước 3: Tìm hiểu race condition

Ví dụ cơ bản:

```cpp
counter++;
```

Thực chất là:

```cpp
int temp = counter;
temp = temp + 1;
counter = temp;
```

Nếu 2 thread cùng cộng, dữ liệu sẽ bị ghi đè. Đó là race condition.

Giải pháp: `std::mutex` + `std::lock_guard<std::mutex>`.

---

## Bước 4: Hiểu vì sao phải dùng mutex

Trong project này, shared resources là:

```cpp
std::vector<int> clients_;
std::unordered_map<int, std::string> client_names_;
```

Vì nhiều client thread cùng truy cập, nên cần lock để tránh:

- data race
- corrupted vector
- invalid map access
- inconsistent room state

---

## Bước 5: Học cách thiết kế project theo OOP

Project nên có rõ API:

```cpp
class ChatServer {
public:
    void start();
    void stop();
private:
    void acceptLoop();
    void handleClient(int client_fd);
    void broadcastMessage(...);
};
```

Bạn cần biết:

- class nào chịu trách nhiệm gì
- state nào là private
- resource nào cần cleanup
- destructor phải giải phóng tài nguyên

---

## Bước 6: Tại sao dùng `std::thread` không phải `pthread`?

`std::thread` là abstraction của C++11+, dễ hơn và tích hợp với STL, exception handling, RAII.

Mỗi kết nối mới có thể tạo 1 thread hoặc có thể dùng thread pool ở mức nâng cao.

---

## Bước 7: Dạng câu hỏi phỏng vấn gợi ý

### C++

- `std::mutex` khác `std::lock_guard` như thế nào?
- Tại sao `std::lock_guard` tốt hơn `mutex.lock()` thủ công?
- Khi nào dùng `std::unique_lock`?
- Khi nào dùng `join()` và `detach()`?
- Tại sao phải tránh deadlock?

### Socket

- `listen` và `accept` khác nhau như thế nào?
- `recv` trả về gì nếu client disconnect?
- `SO_REUSEADDR` dùng để làm gì?
- TCP là connection-oriented, UDP khác gì?

### Design

- Server này có thể mở rộng ra private chat, file transfer, room chat, login system không?
- Nếu có 10k client, bạn sẽ làm gì khác?
- Nếu sử dụng thread pool, làm sao quản lý queue and worker threads?

---

## Bước 8: Nền tảng để nâng cấp

Sau khi project cơ bản chạy được, bạn có thể phát triển thêm:

1. private message
2. room chat
3. user authentication
4. command `/who`, `/kick`, `/help`
5. message queue
6. JSON protocol
7. Qt client GUI

---

## Bước 9: Checklist hoàn thành tốt nghiệp project

- [ ] Build thành công
- [ ] Server chạy trên port 8080
- [ ] Client connect được
- [ ] Username được lưu
- [ ] Message broadcast tới client khác
- [ ] Sender không nhận lại chính mình
- [ ] Disconnect cleanup đúng
- [ ] Dùng mutex bảo vệ shared state
- [ ] Có thể giải thích data flow bằng lời

---

## Bước 10: Cách nói khi phỏng vấn

Bạn nên trả lời kiểu:

> Tôi đã xây dựng một TCP chat server bằng C++17. Server tạo socket, bind port, listen, accept mỗi client rồi chạy 1 thread riêng cho mỗi kết nối. Shared state như danh sách client và username được bảo vệ bằng std::mutex để tránh data race. Khi nhận tin nhắn, server broadcast tới các client khác, và khi disconnect thì server loại client khỏi danh sách và thông báo leave event.

Đây là câu trả lời rất phù hợp cho Junior C++.
