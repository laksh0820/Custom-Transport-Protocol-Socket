# 📦 KTP: Reliable Transport Protocol over UDP
## 📚 Overview

The project implements **KTP (KGP Transport Protocol)** — a **reliable, message-oriented** transport protocol built over **UDP**, mimicking key TCP functionalities like reliable delivery, flow control, retransmissions, and in-order delivery, all while using user-space mechanisms (threads, shared memory, custom socket interface).

---

## 🎯 Objectives

- Emulate **end-to-end reliable data transfer** over **unreliable UDP**
- Maintain **message ordering** and **window-based flow control**
- Implement custom API (`k_socket`, `k_bind`, etc.)
- Simulate message drops using a configurable probability
- Support **multiple application-level sockets** with shared infrastructure

---

## ⚙️ Functional Overview

### ✔️ Core Functionalities

| Function | Description |
|---------|-------------|
| `k_socket()` | Creates a new KTP socket. Initializes shared memory entry, binds UDP socket. |
| `k_bind()` | Binds KTP socket to source and destination IP/Port. Required for all sockets. |
| `k_sendto()` | Sends a 512-byte message. Handles window availability and retransmission buffering. |
| `k_recvfrom()` | Receives a message (if available) from the receiver buffer. |
| `k_close()` | Closes a socket, releases shared memory entry. |
| `dropMessage(p)` | Drops packets with probability `p`. Used for simulating unreliable links. |

### ✔️ Threads & Processes

| Component | Responsibility |
|----------|----------------|
| `Thread R` | Receives UDP messages, updates receiver buffer or swnd, sends ACKs. |
| `Thread S` | Timer-based. Retransmits unacknowledged messages and sends from buffer. |
| `Garbage Collector` | Detects and frees orphan sockets (zombie processes). |

---

## 🧠 Data Structures

### 1. `struct swnd`
Represents the **sending window**.

| Field | Description |
|-------|-------------|
| `int size` | Current window size. |
| `int seq[MAXNMSG]` | Sequence numbers in the window. |
| `int next` | Next sequence number to be assigned. |
| `time_t timer` | Last send time for timeout tracking. |

---

### 2. `struct swnd_buff`
Represents the **sender-side message buffer**.

| Field | Description |
|-------|-------------|
| `int start` | Index of the first unacknowledged message. |
| `int end` | Index to insert the next message. |
| `char data[MAXNMSG][MSGSIZE]` | Data buffer for messages. |

---

### 3. `struct rwnd`
Represents the **receiver window**.

| Field | Description |
|-------|-------------|
| `int size` | Window size = available space in receiver buffer. |
| `int seq[MAXNMSG]` | Sequence numbers expected. |

---

### 4. `struct rwnd_buff`
Represents the **receiver-side message buffer**.

| Field | Description |
|-------|-------------|
| `int read_seq` | Next message sequence number to be read. |
| `int rcv_seq[MAXNMSG]` | Occupancy map: -1 = empty, else seq no. |
| `char data[MAXNMSG][MSGSIZE]` | Received data buffer. |

---

### 5. `struct KTPHeader`
Header for every message.

| Field | Description |
|-------|-------------|
| `int type` | `DATAMSG`, `ACKMSG`, or `QUERYMSG`. |
| `int seq_no` | Message sequence number. |
| `int ack_no` | Acknowledged sequence number. |
| `int rwnd_size` | Receiver window size (buffer availability). |
| `int ref_type` | Reserved for metadata (future use). |

---

### 6. `struct KTPMessage`
Represents a full message: header + payload.

| Field | Description |
|-------|-------------|
| `KTPHeader header` | Metadata. |
| `char data[MSGSIZE]` | 512 bytes of application data. |

---

### 7. `struct ksock_info`
Shared memory representation of a single KTP socket.

| Field | Description |
|-------|-------------|
| `int isalloc` | 1 = allocated, 0 = free. |
| `int pid` | Process ID of the socket creator. |
| `int udp_sockfd` | Underlying UDP socket descriptor. |
| `int isbind` | Whether the socket is bound. |
| `struct sockaddr_in src_addr` | Source address. |
| `struct sockaddr_in dest_addr` | Destination address. |
| `struct swnd_buff send_buff` | Sender-side buffer. |
| `struct rwnd_buff rcv_buff` | Receiver-side buffer. |
| `struct swnd send_info` | Send window tracking. |
| `struct rwnd rcv_info` | Receive window tracking. |

---

## 📊 Experimental Results

### Simulation of Packet Loss using `dropMessage(p)`

| PROB_DROP | TOTAL_TRANSMISSIONS | MESSAGES | AVG_PER_MSG |
|-----------|----------------------|----------|--------------|
| 0.05      | 168                  | 103      | 1.631        |
| 0.10      | 166                  | 103      | 1.612        |
| 0.15      | 211                  | 103      | 2.049        |
| 0.20      | 202                  | 103      | 1.961        |
| 0.25      | 247                  | 103      | 2.398        |
| 0.30      | 277                  | 103      | 2.689        |
| 0.35      | 304                  | 103      | 2.951        |
| 0.40      | 352                  | 103      | 3.417        |
| 0.45      | 360                  | 103      | 3.495        |
| 0.50      | 499                  | 103      | 4.845        |

---

## 🧪 Testing Strategy

Two programs are used to validate the protocol:

### `user1.c`
- Creates KTP socket
- Sends file (>100KB) using `k_sendto`

### `user2.c`
- Creates receiving KTP socket
- Receives the file via `k_recvfrom`

> Multiple parallel sockets can be used for extended testing.

---

## 🏗️ Compilation & Usage

### 💻 Build Instructions
```bash
make libksocket.a      # Build KTP static library
make initksocket       # Initializes threads and memory
make user1             # File sender
make user2             # File receiver
