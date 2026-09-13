# AI Support Project

## Mục đích

File này lưu nhật ký hỗ trợ giữa người dùng và AI cho project
`cpp-chat-server`: yêu cầu, giải thích kỹ thuật, thay đổi mã nguồn, lệnh build
và kết quả kiểm tra.

## Quy ước cập nhật

- Mỗi lượt trao đổi liên quan đến project sẽ được bổ sung vào file này trong
  quá trình AI xử lý yêu cầu.
- Nội dung được tóm tắt theo chủ đề để dễ tra cứu, không sao chép máy móc toàn
  bộ hội thoại.
- AI không thể tự chạy một tiến trình nền để ghi file sau khi một tin nhắn kết
  thúc nếu không có lượt xử lý mới; vì vậy việc cập nhật được thực hiện trong
  từng lượt chat mà AI đang xử lý.

---

## Nhật ký trao đổi

### 2026-09-13 - Tổng hợp project

- Project là TCP multi-client chat server viết bằng C++17 trên Linux.
- Kiến thức chính: POSIX sockets, TCP/IP, `std::thread`, `std::mutex`,
  `std::lock_guard`, OOP, RAII, STL và Makefile.
- Server hiện có username registration, join/leave notification,
  broadcast message, `/help`, `/who` và `/msg`.
- `ChatServer` quản lý socket/lifecycle; `ChatRoom` quản lý client và
  username; client có thread riêng để nhận message.
- `docs/server.cpp` là bản procedural dùng để học, không phải file được
  Makefile hiện tại build.

### 2026-09-13 - Hiển thị message của chính client

- Mong muốn giao diện client A hiển thị message mình gửi dưới dạng:

  ```text
  [You]: hello Linda
  ```

- Không muốn còn dòng input thô `hello Linda` phía trên.
- Message từ client khác vẫn giữ dạng:

  ```text
  [Linda]: hello
  ```

- Cách dự kiến ở `client/src/client.cpp`: sau khi `send()` thành công, dùng
  ANSI escape codes để xóa dòng terminal vừa nhập rồi in lại `[You]: ...`.
- Cần lưu ý receiver thread và input thread cùng ghi terminal nên output có thể
  bị chồng nếu message đến đúng lúc người dùng đang nhập.

### 2026-09-13 - Makefile

- `make` hoặc `make all`: build `bin/chat_server` và `bin/chat_client`.
- `make run-server`: build nếu cần rồi chạy server.
- `make run-client`: build nếu cần rồi chạy client.
- `make clean`: xóa thư mục `bin`.
- `make -n`: xem lệnh mà không thực thi.
- `make -B`: ép build lại.
- Makefile dùng `g++`, C++17, `-Wall`, `-Wextra`, `-pthread` và
  `-Iserver/include`.
- Có thể cải thiện dependency bằng cách khai báo
  `server/include/ChatServer.h` trong target build server.

### 2026-09-13 - Folder mở rộng

- `common/`: code dùng chung giữa server và client, ví dụ protocol,
  message type và utility.
- `scripts/`: script tự động hóa build, run và test; hiện Makefile đã đáp ứng
  phần lớn nhu cầu cơ bản.
- `tests/`: unit test cho `ChatRoom`, command test và integration test
  server-client qua socket.
- Nên ưu tiên test `ChatRoom` trước khi mở rộng protocol hoặc script.

### 2026-09-13 - Đường dẫn Linux

- Lỗi `mv AI_support_Project.md /cpp-chat-server` xảy ra vì `/cpp-chat-server`
  là đường dẫn bên dưới root `/`, không phải project của user.
- Project thật ở:

  ```text
  /home/captain/workspace/cpp-chat-server
  ```

- Từ thư mục `docs`, lệnh đúng để đưa file lên root project là:

  ```bash
  mv AI_support_Project.md ..
  ```
