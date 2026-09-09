# Script trả lời phỏng vấn Junior C++ - Project Chat Server

## 1. Bạn đã làm project gì?

> Tôi đã làm một project chat server bằng C++17 với mô hình TCP socket. Server lắng nghe trên một port, chấp nhận kết nối từ nhiều client, và với mỗi client tạo một thread riêng để xử lý dữ liệu. Khi client gửi tin nhắn, server broadcast tới các client khác. Tôi cũng dùng mutex để bảo vệ dữ liệu shared state như danh sách client và tên user.

---

## 2. Project này dùng những công nghệ gì?

> Project sử dụng C++17, Linux socket API, TCP/IP, std::thread, std::mutex, std::vector, std::unordered_map, và Makefile để build. Đây là một stack rất phù hợp để minh họa kiến thức về backend C++ và hệ thống mạng cơ bản.

---

## 3. Socket hoạt động như thế nào trong project?

> Server tạo socket bằng socket(), bind port, và listen để chờ client. Khi có client kết nối, accept() tạo một socket mới cho kết nối đó. Sau đó server dùng recv() để đọc tin nhắn và send() để gửi tin nhắn tới client hoặc broadcast tới các client khác.

---

## 4. Tại sao phải dùng multithreading?

> Vì server cần xử lý nhiều client đồng thời. Nếu không dùng thread, một client chậm hoặc mất kết nối có thể làm toàn bộ server bị block. Với mỗi client, tôi tạo một thread riêng để xử lý logic kết nối, giúp server đáp ứng nhiều client cùng lúc.

---

## 5. Mutex dùng để làm gì?

> Mutex được dùng để bảo vệ các tài nguyên chung như vector client và map tên user. Khi nhiều thread cùng truy cập, dữ liệu có thể bị race condition. std::lock_guard giúp lock automatic và unlock khi ra khỏi scope, giảm nguy cơ deadlock và lỗi đồng bộ.

---

## 6. Race condition là gì?

> Race condition xảy ra khi nhiều thread cùng đọc và ghi cùng một biến hoặc cấu trúc dữ liệu mà không có synchronization. Ví dụ counter++ không phải là atomic; nó thực chất là đọc, cộng, ghi. Nếu chạy đồng thời, kết quả có thể sai.

---

## 7. Kết nối client disconnect thì server xử lý như thế nào?

> Server kiểm tra recv() trả về 0, nghĩa là client đã đóng kết nối. Khi đó server xóa client khỏi danh sách, xóa tên user trong map, và broadcast thông báo người đó rời phòng. Sau đó close socket để giải phóng tài nguyên.

---

## 8. Một thread có thể nguy hiểm không? Có phải lúc nào cũng dùng detach không?

> Không phải lúc nào cũng nên detach. detach dùng cho thread chạy nền khi tôi không cần chờ kết quả. Nếu cần đồng bộ hoặc kiểm soát lifecycle, tôi dùng join. Trong project này, mỗi client thread là background task nên detach phù hợp. Nhưng cần cẩn thận vì nếu không quản lý đúng, có thể dẫn tới race condition hoặc resource leak.

---

## 9. Nếu bạn nói với interviewer về design, bạn sẽ nói gì?

> Tôi thiết kế server theo mô hình OOP: class ChatServer chịu trách nhiệm tạo socket, bắt đầu accept loop, xử lý từng client, và broadcast message. Điều này giúp tách biệt rõ trách nhiệm: server làm network layer, handleClient làm business logic, broadcastMessage làm phần truyền tin. Cách này dễ mở rộng và dễ test hơn.

---

## 10. Nếu có thêm thời gian, bạn muốn cải tiến project như thế nào?

> Tôi muốn nâng cấp project bằng cách thêm room chat, private message, user authentication, command system, và sử dụng thread pool thay vì tạo thread cho mỗi client. Sau đó có thể làm giao diện Qt client để thấy server và client tương tác trực tiếp. Đây là hướng phù hợp để mở rộng lên mức system backend thực tế hơn.

---

## Câu trả lời ngắn gọn cho interviewer

> Tôi đã xây dựng một TCP chat server bằng C++17. Server tạo socket, bind port, listen để chờ client, accept từng kết nối và tạo thread xử lý cho từng client. Shared state như danh sách client và tên người dùng được bảo vệ bởi mutex để tránh race condition. Khi nhận tin nhắn, server broadcast tới các client khác, và khi client disconnect thì xoá khỏi danh sách và thông báo rời phòng. Đây là một project rất phù hợp để thể hiện kiến thức về socket programming, multithreading và synchronization trong C++.
