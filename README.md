# Multi Client Chat Server

## Overview

A professional C++ backend project focused on:

- Linux System Programming
- TCP/IP Socket Programming
- Multithreading
- Synchronization (Mutex)
- Design Patterns
- Object-Oriented Design
- Git Workflow

The goal is to build a scalable multi-client chat server that can later be extended with a Qt GUI client.

---

# Technology Stack

## Core

- C++17
- STL
- Linux
- POSIX Socket API

## Networking

- socket()
- bind()
- listen()
- accept()
- recv()
- send()
- close()

## Concurrency

- std::thread
- std::mutex
- std::lock_guard

## Data Structures

- std::vector
- std::unordered_map
- std::string

## Tools

- g++
- Makefile
- Git
- GitHub

## Future

- Qt Widgets
- QTcpSocket
- Design Patterns
- Thread Pool

---

# Current Features

## Server

- TCP Server
- Multi Client Support
- One Thread Per Client
- Username Registration
- Join Notification
- Leave Notification
- Message Broadcasting

## Thread Safety

Protected shared resources:

```cpp
std::vector<int> clients;
std::unordered_map<int, std::string> client_names;
std::mutex clients_mutex;
```

---

# Core Workflow

## Connection

```text
Client Connect
        ↓
accept()
        ↓
Create Thread
        ↓
handle_client()
```

---

## Username Registration

```text
Server
  ↓
"Enter your name"
  ↓
Client sends username
  ↓
Store in client_names
```

Example:

```cpp
client_names[4] = "Steven";
client_names[5] = "Linda";
```

---

## Chat Message Flow

```text
Client A
  ↓
recv()
  ↓
Server
  ↓
broadcast_message()
  ↓
Client B
Client C
Client D
```

Sender does not receive its own message.

---

## Disconnect Flow

```text
Client disconnect
      ↓
Remove from clients
      ↓
Remove from client_names
      ↓
Broadcast leave notification
      ↓
close(client_fd)
```

---

# Important Multithreading Concepts Learned

## Race Condition

Example:

```cpp
counter++;
```

is actually:

```cpp
temp = counter;
temp = temp + 1;
counter = temp;
```

Multiple threads may overwrite each other.

---

## Mutex Protection

```cpp
std::lock_guard<std::mutex> lock(clients_mutex);
```

Protects:

```cpp
clients
client_names
```

from concurrent modification.

---

## Deadlock

Potential issue when:

```cpp
mutexA
mutexB
```

are acquired in different orders by different threads.

Current project uses a single mutex to avoid deadlocks.

---

## Thread Lifecycle

### join()

```cpp
thread.join();
```

Main thread waits.

---

### detach()

```cpp
thread.detach();
```

Thread runs independently.

Used for client handler threads.

---

# Current Project Structure (Target)

```text
chat_system/
│
├── client/
│   ├── src/
│   ├── include/
│   └── Makefile
│
├── server/
│   ├── src/
│   ├── include/
│   └── Makefile
│
├── common/
│   ├── protocol/
│   └── utils/
│
├── config/
│
├── logs/
│
├── docs/
│
├── tests/
│
├── scripts/
│
├── Makefile
│
└── README.md
```

---

# Planned Refactoring

Current implementation is mostly inside:

```cpp
server.cpp
```

Next step:

```text
server/
│
├── ChatRoom.h
├── ChatRoom.cpp
│
├── ClientSession.h
├── ClientSession.cpp
│
└── server.cpp
```

---

# Design Patterns Roadmap

## Phase 1 (Completed)

```text
Socket
Thread
Mutex
Broadcast
Username
```

---

## Phase 2 (Next)

### Observer Pattern

ChatRoom acts as Subject.

Clients act as Observers.

```text
ChatRoom
 ├── Steven
 ├── Linda
 └── Alice
```

When a message arrives:

```cpp
notifyAll();
```

---

## Phase 3

Command System

Examples:

```text
/list
/quit
/msg Steven hello
```

Potential patterns:

- Command Pattern

---

## Phase 4

Thread Pool

Replace:

```cpp
One Thread Per Client
```

with:

```text
Worker Thread Pool
```

Benefits:

- Better scalability
- Lower memory usage
- Reduced context switching

---

## Phase 5

Qt GUI Client

Replace terminal client:

```bash
nc 127.0.0.1 8080
```

with:

```text
Qt Chat Application
```

Technologies:

- Qt Widgets
- QTcpSocket

---

# CV Summary

## Multi Client Chat Server

Technology:
C++, STL, Linux, TCP/IP Socket, Multithreading, Mutex, Git

Responsibilities:

- Designed and implemented a multi-client chat server using TCP sockets.
- Developed concurrent client handling using std::thread.
- Implemented thread-safe shared resources using mutex synchronization.
- Developed real-time message broadcasting between connected clients.
- Implemented username registration and session management.
- Applied object-oriented design principles for scalable architecture.
- Prepared architecture for Observer Pattern, Thread Pool, and Qt GUI integration.