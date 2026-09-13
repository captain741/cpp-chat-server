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

### 2026-09-13 - Đánh giá và roadmap nâng cấp

Đánh giá hiện tại: project đã đạt mức prototype tốt cho việc học C++ system
programming và có thể trình bày trong phỏng vấn Junior. Kiến trúc đã có
`ChatServer`/`ChatRoom`, TCP, nhiều client, mutex, command và private message.
Tuy nhiên chưa nên gọi là production-ready vì protocol, lifecycle thread,
error handling, test và bảo mật còn đơn giản.

Thứ tự ưu tiên đề xuất:

1. **Ổn định protocol TCP**
   - Thêm framing rõ ràng, ví dụ newline-delimited message.
   - Xử lý trường hợp một `recv()` nhận nhiều message hoặc chỉ nhận một phần.
   - Viết `sendAll()` để xử lý partial send.
   - Giới hạn kích thước message và validate username/message.

2. **Sửa concurrency và shutdown**
   - Đổi `running_` từ `bool` sang `std::atomic<bool>`.
   - Quản lý client thread thay vì detach hoàn toàn, hoặc tạo cơ chế stop
     session rõ ràng.
   - Xem xét giảm vùng giữ mutex; không giữ mutex trong lúc `send()` lâu.
   - Kiểm tra race bằng ThreadSanitizer nếu môi trường hỗ trợ.

3. **Hoàn thiện quản lý user**
   - Không cho phép username rỗng hoặc trùng.
   - Chuẩn hóa username và giới hạn độ dài.
   - Xử lý client mất kết nối giữa lúc broadcast.
   - Có thể bổ sung `/quit`, `/kick` và quyền admin sau khi nền tảng ổn định.

4. **Tách kiến trúc và tăng khả năng test**
   - Tách `ChatRoom`, `CommandHandler`, protocol và utility thành module riêng.
   - Tạo unit test cho `ChatRoom` và parser command.
   - Tạo integration test cho hai client chat, private message và disconnect.
   - Cập nhật Makefile để build object files và test target.

5. **Logging và cấu hình**
   - Tách log server khỏi `std::cout`, thêm timestamp và level.
   - Cho phép cấu hình port, backlog, message limit từ command line hoặc file.
   - Tránh hard-code `127.0.0.1`, `8080` và buffer size ở nhiều nơi.

6. **Bảo mật và độ tin cậy**
   - Thêm authentication nếu project cần tài khoản thật.
   - Không log password/token.
   - Xử lý `SIGPIPE` hoặc dùng cờ phù hợp khi send.
   - Có rate limit cơ bản để một client không spam server.

7. **Khả năng mở rộng**
   - Với vài client, one-thread-per-client vẫn phù hợp.
   - Khi cần nhiều client, chuyển sang thread pool hoặc event loop bằng
     `poll`, `epoll` hoặc Boost.Asio.
   - Sau đó mới cân nhắc JSON protocol, database, room chat và Qt GUI.

Các việc nên làm ngay theo thứ tự ngắn hạn:

```text
1. message framing + sendAll
2. username validation/duplicate check
3. atomic running_ và shutdown an toàn
4. unit tests cho ChatRoom/command parser
5. cập nhật Makefile và README theo code thực tế
```
