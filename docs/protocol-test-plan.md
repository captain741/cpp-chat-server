# Protocol Test Plan

## Phạm vi

Tài liệu này kiểm thử giai đoạn 1 của chat server:

- TCP message framing.
- Partial `send()` và partial `recv()`.
- Nhiều message trong một TCP segment.
- Message bị chia thành nhiều TCP segment.
- Username và message ở biên kích thước.
- Command và message có ký tự xuống dòng.
- Disconnect và lỗi socket.

## Quy ước protocol đề xuất

Mỗi frame là một dòng UTF-8 kết thúc bằng `\n`:

```text
<payload>\n
```

Server phải tích lũy bytes theo từng connection, tách tất cả frame hoàn chỉnh
và giữ phần chưa hoàn chỉnh cho lần `recv()` tiếp theo. Message không có
newline cuối chưa được xem là frame hoàn chỉnh.

Giới hạn đề xuất:

- Username: 1-32 bytes sau khi trim.
- Chat message: 1-1024 bytes.
- Một frame vượt giới hạn phải bị từ chối mà không làm crash hoặc ngắt các
  client khác.

## Tiêu chí chung

- Không crash, deadlock hoặc leak socket/thread.
- Không trộn hai message thành một message.
- Không làm mất message hợp lệ.
- Sender không nhận broadcast thường của chính mình.
- Client khác nhận đúng thứ tự message.
- Lỗi một client không làm ảnh hưởng client còn lại.

## Danh sách test case

| ID | Mức độ | Kịch bản | Kết quả mong đợi |
|---|---|---|---|
| P-001 | P0 | Gửi một username và một message hợp lệ | Server đăng ký đúng username và broadcast đúng payload |
| P-002 | P0 | Gửi một frame thành nhiều lần `send()` với delay giữa các phần | Server ghép đủ bytes thành đúng một frame |
| P-003 | P0 | Gửi hai hoặc nhiều frame trong một lần `send()` | Server tách và xử lý từng message riêng theo đúng thứ tự |
| P-004 | P0 | Gửi frame rỗng `\n` | Server bỏ qua hoặc trả lỗi rõ ràng; không broadcast message rỗng |
| P-005 | P0 | Gửi message thiếu newline rồi đóng socket | Server không xử lý phần chưa hoàn chỉnh như message hợp lệ |
| P-006 | P0 | Gửi message dài đúng giới hạn | Server xử lý đầy đủ, không cắt payload |
| P-007 | P0 | Gửi message vượt giới hạn một chút | Server từ chối frame và vẫn phục vụ connection/client khác |
| P-008 | P0 | Gửi nhiều frame có tổng kích thước lớn hơn buffer `recv()` | Tất cả frame vẫn được tách đúng, không phụ thuộc kích thước buffer |
| P-009 | P0 | Client B đóng ngay khi server broadcast | Server xử lý lỗi gửi, không crash và vẫn broadcast cho C |
| P-010 | P0 | Client đóng giữa một frame | Server cleanup connection, không broadcast frame dở dang |
| P-011 | P1 | Username có `\r\n`, space đầu/cuối | Trim theo policy; không lưu ký tự điều khiển hoặc khoảng trắng ngoài ý muốn |
| P-012 | P1 | Username rỗng hoặc toàn whitespace | Từ chối đăng ký, đóng hoặc yêu cầu nhập lại theo policy |
| P-013 | P1 | Hai client dùng cùng username | Client thứ hai bị từ chối; `/who` không có tên trùng |
| P-014 | P1 | Username dài hơn giới hạn | Từ chối rõ ràng, không overflow/truncate âm thầm |
| P-015 | P1 | Message chứa `\r\n` giữa payload | Không cho phép tạo fake message/system event; xử lý theo protocol đã định |
| P-016 | P1 | Gửi `/help\n` | Nhận đúng help response, không broadcast như chat message |
| P-017 | P1 | Gửi `/who\n` khi không có/đang có user | Kết quả đúng và không deadlock khi user join/leave đồng thời |
| P-018 | P1 | Gửi `/msg Bob hello\n` | Chỉ Bob và sender nhận đúng private-message response |
| P-019 | P1 | Gửi `/msg Missing hello\n` | Sender nhận `User not found`, không broadcast |
| P-020 | P1 | Gửi command `/msg` sai format | Nhận usage/error rõ ràng, server tiếp tục nhận message sau đó |
| P-021 | P1 | Gửi unknown command | Nhận `Unknown command`, không crash/broadcast |
| P-022 | P1 | Gửi nhiều command trong một `send()` | Từng command được xử lý độc lập theo thứ tự |
| P-023 | P1 | Gửi byte `\0` trong frame | Server không coi đó là kết thúc chuỗi sớm; từ chối hoặc xử lý theo policy |
| P-024 | P1 | Gửi dữ liệu không phải UTF-8 | Từ chối hoặc giữ nguyên bytes theo protocol; không crash |
| P-025 | P1 | Client gửi liên tục tốc độ cao | Không mất/trộn frame; có giới hạn hoặc backpressure rõ ràng |
| P-026 | P1 | Nhiều client cùng gửi message | Mỗi client nhận message hợp lệ; thứ tự được định nghĩa và nhất quán |
| P-027 | P1 | Gửi `shutdown` server trong lúc client đang `recv()` | Client thoát rõ ràng; server join/cleanup đúng, không treo |
| P-028 | P2 | Kết nối rồi không gửi username | Server không giữ tài nguyên vô hạn; có timeout hoặc policy rõ ràng |
| P-029 | P2 | Kết nối rồi gửi username cực chậm | Không làm block accept loop hoặc các client khác |
| P-030 | P2 | Mở nhiều client vượt backlog | Server xử lý trong giới hạn tài nguyên, lỗi được báo rõ |

## Cách chạy thủ công

Build:

```bash
make -C /home/captain/workspace/cpp-chat-server clean
make -C /home/captain/workspace/cpp-chat-server all
```

Chạy server:

```bash
cd /home/captain/workspace/cpp-chat-server
./bin/chat_server
```

Client thường:

```bash
./bin/chat_client
```

Để kiểm thử framing chính xác, nên dùng một script socket thay vì chỉ dùng
terminal client, vì terminal client hiện không cho kiểm soát từng lần `send()`.

## Các kịch bản chạy trực tiếp trên terminal

Các lệnh dưới đây dùng Python 3 và chỉ sử dụng thư viện chuẩn. Mở Terminal 1
để chạy server:

```bash
cd /home/captain/workspace/cpp-chat-server
make all
./bin/chat_server
```

### T-001: Hai client chat bình thường

Mở Terminal 2:

```bash
cd /home/captain/workspace/cpp-chat-server
./bin/chat_client
```

Nhập username `Alice`. Mở Terminal 3 và chạy client thứ hai, nhập `Bob`.
Từ Alice gửi:

```text
hello Bob
```

Kết quả mong đợi ở Bob:

```text
[Alice]: hello Bob
```

Không được xuất hiện hai message, message rỗng hoặc ký tự lạ.

### T-002: Một message bị chia thành hai lần `send()`

Giữ server chạy ở Terminal 1. Mở Terminal 2 để làm receiver:

```bash
python3 - <<'PY'
import socket
import time

s = socket.create_connection(("127.0.0.1", 8080))
print(s.recv(256).decode(errors="replace"), end="")
s.sendall(b"Receiver\n")
time.sleep(0.5)
while True:
    data = s.recv(4096)
    if not data:
        break
    print(data.decode(errors="replace"), end="")
PY
```

Mở Terminal 3 để gửi message bị chia:

```bash
python3 - <<'PY'
import socket
import time

s = socket.create_connection(("127.0.0.1", 8080))
print(s.recv(256).decode(errors="replace"), end="")
s.sendall(b"Sender\n")
time.sleep(0.5)
s.sendall(b"hello ")
time.sleep(1)
s.sendall(b"Receiver\n")
time.sleep(1)
s.close()
PY
```

Kết quả đúng phải chỉ có một dòng:

```text
[Sender]: hello Receiver
```

Nếu receiver thấy `[Sender]: hello` rồi một dòng riêng `[Sender]: Receiver`,
protocol hiện tại đang xử lý mỗi `recv()` như một message và test bị fail.

### T-003: Nhiều message trong một lần `send()`

Mở một receiver như T-002, sau đó chạy sender:

```bash
python3 - <<'PY'
import socket
import time

s = socket.create_connection(("127.0.0.1", 8080))
print(s.recv(256).decode(errors="replace"), end="")
s.sendall(b"Sender\n")
time.sleep(0.5)
s.sendall(b"first\nsecond\n")
time.sleep(1)
s.close()
PY
```

Kết quả đúng:

```text
[Sender]: first
[Sender]: second
```

Nếu receiver nhận một message chứa cả `first\nsecond`, test fail vì server chưa
tách nhiều frame trong cùng buffer.

### T-004: Message không có newline rồi đóng socket

Chạy receiver như T-002, sau đó:

```bash
python3 - <<'PY'
import socket
import time

s = socket.create_connection(("127.0.0.1", 8080))
print(s.recv(256).decode(errors="replace"), end="")
s.sendall(b"Sender\n")
time.sleep(0.5)
s.sendall(b"incomplete message")
time.sleep(0.5)
s.close()
```

Theo protocol newline-delimited, receiver không nên nhận message chưa hoàn
chỉnh. Nếu vẫn nhận `[Sender]: incomplete message`, server đang xử lý frame dở
dang như message hoàn chỉnh.

### T-005: Message lớn hơn buffer `recv()`

Mở receiver như T-002, sau đó gửi payload dài:

```bash
python3 - <<'PY'
import socket
import time

s = socket.create_connection(("127.0.0.1", 8080))
print(s.recv(256).decode(errors="replace"), end="")
s.sendall(b"Sender\n")
time.sleep(0.5)
s.sendall(b"A" * 1500 + b"\n")
time.sleep(1)
s.close()
PY
```

Kết quả đúng sau khi có giới hạn protocol là server phải từ chối rõ ràng hoặc
xử lý theo policy, không được cắt thành nhiều message giả và không được crash.
Implementation hiện tại có thể tạo nhiều message do buffer chỉ có 1024 bytes.

### T-006: Username chứa khoảng trắng và ký tự xuống dòng

Chạy:

```bash
python3 - <<'PY'
import socket
import time

s = socket.create_connection(("127.0.0.1", 8080))
print(s.recv(256).decode(errors="replace"), end="")
s.sendall(b"  Alice  \r\n")
time.sleep(1)
s.close()
```

Kiểm tra log server và `/who` từ một client khác. Username đúng theo policy
phải là `Alice`, không phải chuỗi có space hoặc `\r`.

### T-007: Hai username trùng nhau

Mở hai client/script và gửi lần lượt:

```text
Alice
Alice
```

Kết quả đúng sau khi bổ sung validation: client thứ hai bị từ chối hoặc được
yêu cầu chọn tên khác. Nếu `/who` hiển thị hai `Alice`, test fail.

### T-008: Nhiều command trong một lần `send()`

Mở một script client đã đăng ký username rồi gửi:

```bash
python3 - <<'PY'
import socket
import time

s = socket.create_connection(("127.0.0.1", 8080))
print(s.recv(256).decode(errors="replace"), end="")
s.sendall(b"CommandTester\n")
time.sleep(0.5)
s.sendall(b"/help\n/who\n")
time.sleep(1)
print(s.recv(4096).decode(errors="replace"), end="")
s.close()
PY
```

Kết quả đúng là hai response riêng theo đúng thứ tự. Nếu server trả một
`Unknown command` cho chuỗi gộp, test fail.

### T-009: Client đóng khi server broadcast

Mở receiver bằng T-002, đăng ký `Receiver`, sau đó đóng socket receiver ngay.
Trong Terminal khác chạy sender:

```bash
python3 - <<'PY'
import socket
import time

s = socket.create_connection(("127.0.0.1", 8080))
print(s.recv(256).decode(errors="replace"), end="")
s.sendall(b"Sender\n")
time.sleep(0.5)
s.sendall(b"message after receiver closed\n")
time.sleep(1)
s.close()
PY
```

Server không được crash hoặc dừng accept client mới. Kiểm tra bằng cách mở
thêm `./bin/chat_client` sau test.

### T-010: Kiểm tra client còn hoạt động sau lỗi protocol

Sau mỗi test fail, mở client mới:

```bash
cd /home/captain/workspace/cpp-chat-server
./bin/chat_client
```

Nếu client mới không kết nối được, server đã bị crash, deadlock hoặc đóng
listening socket; đây là lỗi P0 cần sửa trước các tính năng khác.

## Bộ test smoke tối thiểu trước mỗi lần thay đổi protocol

Phải chạy ít nhất:

```text
P-001, P-002, P-003, P-005, P-006, P-007,
P-009, P-010, P-013, P-016, P-018, P-022, P-027
```

## Các rủi ro đã thấy từ implementation hiện tại

Implementation hiện đọc một lần `recv()` thành một message và chỉ bỏ một
newline cuối buffer. Vì vậy có khả năng:

- P-002 fail: frame bị chia nhỏ bị xử lý thiếu.
- P-003 fail: nhiều frame bị gộp thành một message.
- P-005 có thể xử lý dữ liệu dở dang như message hợp lệ.
- P-008 fail khi dữ liệu lớn hơn buffer.
- P-011 có thể còn `\r` trong username/message.
- `send()` hiện chưa có vòng lặp xử lý partial send.

Đây là expected failures cần chuyển thành implementation tasks, không nên sửa
test để phù hợp với behavior lỗi.
