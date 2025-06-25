# KTP Socket Project - Reliable Transport Protocol over UDP

![Network Protocol](https://img.shields.io/badge/Protocol-UDP%2BReliability-green)
![Language](https://img.shields.io/badge/Language-C-blue)
![License](https://img.shields.io/badge/License-MIT-orange)

## 📝 Table of Contents
- [Project Overview](#-project-overview)
- [Key Features](#-key-features)
- [Technical Specifications](#-technical-specifications)
- [Performance Metrics](#-performance-metrics)
- [API Documentation](#-api-documentation)
- [System Architecture](#-system-architecture)
- [Data Structures](#-data-structures)
- [Build & Execution](#-build--execution)
- [Testing](#-testing)
- [Contributor](#-contributor)

## 🌟 Project Overview
Implementation of **KTP (KGP Transport Protocol)** - a reliable, message-oriented transport layer protocol built over UDP with sliding window flow control, designed for CS39006 Networks Lab Assignment 4.

## ✨ Key Features
- ✅ Reliable, in-order message delivery
- 🪟 Window-based flow control (window size = 10 messages)
- 🔄 Automatic retransmission on timeout (T=5sec)
- 📶 Supports multiple concurrent sockets (max 10)
- 🧹 Garbage collection for orphaned sockets
- 📉 Configurable drop probability for testing

## ⚙️ Technical Specifications

| Parameter          | Value               |
|--------------------|---------------------|
| Message Size       | 512 bytes           |
| Max Sequence No.   | 255                 |
| Window Size        | 10 messages         |
| Timeout (T)        | 5 seconds           |
| Max Sockets        | 10                  |
| Shared Memory Key  | 101                 |
| Semaphore Key      | 102                 |

## 📊 Performance Metrics

Tested with varying drop probabilities (100KB file transfer):

| Drop Probability (p) | Total Transmissions | Messages Sent | Avg Transmissions/Msg |
|----------------------|---------------------|---------------|-----------------------|
| 0.05                 | 168                 | 103           | 1.631                 |
| 0.10                 | 166                 | 103           | 1.612                 |
| 0.15                 | 211                 | 103           | 2.049                 |
| 0.20                 | 202                 | 103           | 1.961                 |
| 0.25                 | 247                 | 103           | 2.398                 |
| 0.30                 | 277                 | 103           | 2.689                 |
| 0.35                 | 304                 | 103           | 2.951                 |
| 0.40                 | 352                 | 103           | 3.417                 |
| 0.45                 | 360                 | 103           | 3.495                 |
| 0.50                 | 499                 | 103           | 4.845                 |

## 📚 API Documentation

### Core Functions
```c
/* Creates a KTP socket */
int k_socket(int domain, int type, int protocol);

/* Binds socket to source and destination addresses */
int k_bind(int ksockfd, const struct sockaddr_in src_addr, 
          socklen_t src_addrlen, const struct sockaddr_in dest_addr,
          socklen_t dest_addrlen);

/* Sends a message reliably */
ssize_t k_sendto(int ksockfd, char buff[], size_t len, int flags,
                const struct sockaddr_in dest_addr, socklen_t dest_addrlen);

/* Receives a message */
ssize_t k_recvfrom(int ksockfd, char buff[], size_t len, int flags);

/* Closes the KTP socket */
int k_close(int fd);
