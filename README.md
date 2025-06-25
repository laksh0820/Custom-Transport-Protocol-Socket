<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>KTP Socket Project Documentation</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            line-height: 1.6;
            margin: 0;
            padding: 20px;
            color: #333;
        }
        h1, h2, h3 {
            color: #2c3e50;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin: 20px 0;
        }
        th, td {
            border: 1px solid #ddd;
            padding: 8px;
            text-align: center;
        }
        th {
            background-color: #f2f2f2;
        }
        code {
            background-color: #f4f4f4;
            padding: 2px 5px;
            border-radius: 3px;
            font-family: monospace;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        .section {
            margin-bottom: 30px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>KTP Socket Project Documentation</h1>
        
        <div class="section">
            <h2>Project Overview</h2>
            <p>This project implements a reliable transport protocol called KTP (KGP Transport Protocol) on top of UDP sockets. KTP provides reliable, in-order message delivery with window-based flow control, similar to TCP but message-oriented rather than byte-oriented.</p>
        </div>

        <div class="section">
            <h2>Assignment Information</h2>
            <p><strong>Assignment 4 Submission</strong></p>
            <p><strong>Name:</strong> Lakshya Agrawal</p>
            <p><strong>Roll number:</strong> 22CS30036</p>
        </div>

        <div class="section">
            <h2>Performance Results</h2>
            <p>The following table shows the performance of the KTP protocol with varying drop probabilities (p):</p>
            
            <table>
                <tr>
                    <th>PROB_DROP</th>
                    <th>TOTAL_TRANSMISSIONS</th>
                    <th>NO_OF_MSG_GENERATED_FROM_FILE</th>
                    <th>AVG_TRANSMISSIONS_PER_MSG</th>
                </tr>
                <tr><td>0.05</td><td>168</td><td>103</td><td>1.631</td></tr>
                <tr><td>0.10</td><td>166</td><td>103</td><td>1.612</td></tr>
                <tr><td>0.15</td><td>211</td><td>103</td><td>2.049</td></tr>
                <tr><td>0.20</td><td>202</td><td>103</td><td>1.961</td></tr>
                <tr><td>0.25</td><td>247</td><td>103</td><td>2.398</td></tr>
                <tr><td>0.30</td><td>277</td><td>103</td><td>2.689</td></tr>
                <tr><td>0.35</td><td>304</td><td>103</td><td>2.951</td></tr>
                <tr><td>0.40</td><td>352</td><td>103</td><td>3.417</td></tr>
                <tr><td>0.45</td><td>360</td><td>103</td><td>3.495</td></tr>
                <tr><td>0.50</td><td>499</td><td>103</td><td>4.845</td></tr>
            </table>
        </div>

        <div class="section">
            <h2>Global Variables</h2>
            <ul>
                <li><code>int semid</code>: Semaphore ID for ensuring mutual exclusion</li>
                <li><code>struct sembuf pop, vop</code>: Structures for semaphore operations (P and V)</li>
                <li><code>int shmid</code>: Shared memory ID for socket information</li>
                <li><code>int nospace[MAXNKSOCK]</code>: Array for "no space" flags</li>
                <li><code>struct ksock_info *SM</code>: Pointer to shared memory for socket info</li>
            </ul>
        </div>

        <div class="section">
            <h2>Macros</h2>
            <ul>
                <li><code>T</code>: Constant value 5 (timeout in seconds)</li>
                <li><code>PROB_DROP</code>: Probability of dropping a message (0.05)</li>
                <li><code>KEY</code>: Shared memory key (101)</li>
                <li><code>SEMKEY</code>: Semaphore key (102)</li>
                <li><code>MAXNKSOCK</code>: Maximum number of KTP sockets (10)</li>
                <li><code>MAXNMSG</code>: Maximum number of messages (10)</li>
                <li><code>MAXSEQNUM</code>: Maximum sequence number (255)</li>
                <li><code>MSGSIZE</code>: Size of a message (512 bytes)</li>
                <li><code>DATAMSG</code>: Message type for data (0)</li>
                <li><code>ACKMSG</code>: Message type for acknowledgment (1)</li>
                <li><code>QUERYMSG</code>: Message type for query (2)</li>
                <li><code>SOCK_KTP</code>: KTP socket identifier (100)</li>
                <li><code>ENOSPACE</code>: Error code for no space (200)</li>
                <li><code>ENOTBOUND</code>: Error code for socket not bound (201)</li>
                <li><code>ENOMESSAGE</code>: Error code for no message available (202)</li>
            </ul>
        </div>

        <div class="section">
            <h2>Data Structures</h2>
            
            <h3>1. struct swnd</h3>
            <p><strong>Purpose:</strong> Represents the sending window for managing sequence numbers and timers</p>
            <p><strong>Fields:</strong></p>
            <ul>
                <li><code>int size</code>: Current window size</li>
                <li><code>int seq[MAXNMSG]</code>: Sequence numbers in window</li>
                <li><code>int next</code>: Next sequence number to use</li>
                <li><code>time_t timer</code>: Timer for messages</li>
            </ul>

            <h3>2. struct swnd_buff</h3>
            <p><strong>Purpose:</strong> Manages sender's message buffer for unacknowledged messages</p>
            <p><strong>Fields:</strong></p>
            <ul>
                <li><code>int start</code>: Oldest unacknowledged message index</li>
                <li><code>int end</code>: Index for new messages</li>
                <li><code>char data[MAXNMSG][MSGSIZE]</code>: Message buffer</li>
            </ul>

            <h3>3. struct rwnd</h3>
            <p><strong>Purpose:</strong> Represents the receiving window for expected sequence numbers</p>
            <p><strong>Fields:</strong></p>
            <ul>
                <li><code>int size</code>: Current window size</li>
                <li><code>int seq[MAXNMSG]</code>: Expected sequence numbers</li>
            </ul>

            <h3>4. struct rwnd_buff</h3>
            <p><strong>Purpose:</strong> Manages receiver's message buffer for received but unread messages</p>
            <p><strong>Fields:</strong></p>
            <ul>
                <li><code>int read_seq</code>: Next message sequence to read</li>
                <li><code>int rcv_seq[MAXNMSG]</code>: Buffer slot occupancy</li>
                <li><code>char data[MAXNMSG][MSGSIZE]</code>: Received messages</li>
            </ul>

            <h3>5. struct ksock_info</h3>
            <p><strong>Purpose:</strong> Stores information about each KTP socket</p>
            <p><strong>Fields:</strong></p>
            <ul>
                <li><code>int isalloc</code>: Socket allocation status</li>
                <li><code>int pid</code>: Process ID</li>
                <li><code>int udp_sockfd</code>: UDP socket file descriptor</li>
                <li><code>int isbind</code>: Binding status</li>
                <li><code>struct sockaddr_in src_addr</code>: Source address</li>
                <li><code>struct sockaddr_in dest_addr</code>: Destination address</li>
                <li><code>struct swnd_buff send_buff</code>: Sender's buffer</li>
                <li><code>struct rwnd_buff rcv_buff</code>: Receiver's buffer</li>
                <li><code>struct swnd send_info</code>: Sending window info</li>
                <li><code>struct rwnd rcv_info</code>: Receiving window info</li>
            </ul>

            <h3>6. struct KTPHeader</h3>
            <p><strong>Purpose:</strong> Header of a KTP message</p>
            <p><strong>Fields:</strong></p>
            <ul>
                <li><code>int type</code>: Message type (DATAMSG, ACKMSG, QUERYMSG)</li>
                <li><code>int seq_no</code>: Sequence number</li>
                <li><code>int ack_no</code>: Acknowledgment number</li>
                <li><code>int rwnd_size</code>: Receiver window size</li>
                <li><code>int ref_type</code>: Additional metadata</li>
            </ul>

            <h3>7. struct KTPMessage</h3>
            <p><strong>Purpose:</strong> Complete KTP message</p>
            <p><strong>Fields:</strong></p>
            <ul>
                <li><code>struct KTPHeader header</code>: Message header</li>
                <li><code>char data[MSGSIZE]</code>: Payload data</li>
            </ul>
        </div>

        <div class="section">
            <h2>Function Prototypes</h2>
            <ul>
                <li><code>int k_socket(int domain, int type, int protocol)</code>: Creates a KTP socket</li>
                <li><code>int k_bind(int ksockfd, const struct sockaddr_in src_addr, socklen_t src_addrlen, const struct sockaddr_in dest_addr, socklen_t dest_addrlen)</code>: Binds a KTP socket</li>
                <li><code>ssize_t k_sendto(int ksockfd, char buff[], size_t len, int flags, const struct sockaddr_in dest_addr, socklen_t dest_addrlen)</code>: Sends a message</li>
                <li><code>ssize_t k_recvfrom(int ksockfd, char buff[], size_t len, int flags)</code>: Receives a message</li>
                <li><code>int k_close(int fd)</code>: Closes a KTP socket</li>
                <li><code>int dropMessage(float p)</code>: Simulates message dropping</li>
                <li><code>const char *errmsg(int err)</code>: Returns error message</li>
            </ul>
        </div>

        <div class="section">
            <h2>Auxiliary Functions</h2>
            <ul>
                <li><code>int isFull(struct ksock_info* SM, int ksockfd)</code>: Checks if sender's buffer is full</li>
                <li><code>int isempty(struct ksock_info* SM, int ksockfd)</code>: Checks if sender's buffer is empty</li>
                <li><code>int max(int a, int b)</code>: Returns maximum of two integers</li>
                <li><code>void send_msg(int ksockfd, int type, int seq_no, int ack_no, int rwnd_size, int ref_type, char data[])</code>: Sends a message</li>
                <li><code>void rwnd_slide(int ksockfd, int end)</code>: Updates receiving window</li>
                <li><code>void swnd_slide(int ksockfd, int end)</code>: Updates sending window</li>
                <li><code>int getfreeslot(int rcv_seq[])</code>: Finds free slot in receiving buffer</li>
                <li><code>int getlastinorderidx(int rcv_seq[], int seq[])</code>: Finds last in-order message</li>
                <li><code>void garbage_collector()</code>: Cleans up sockets</li>
                <li><code>int is_process_running(int pid)</code>: Checks process status</li>
                <li><code>void cleanup_ksocket(int fd)</code>: Cleans KTP socket</li>
            </ul>
        </div>

        <div class="section">
            <h2>Thread Functions</h2>
            <ul>
                <li><code>void *thread_R()</code>: Handles received messages</li>
                <li><code>void *thread_S()</code>: Handles timeouts and retransmissions</li>
            </ul>
        </div>
    </div>
</body>
</html>
