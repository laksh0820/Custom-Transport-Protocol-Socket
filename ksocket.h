/*
    ============================================================
    Assignment 4 Submission
    Name: Lakshya Agrawal
    Roll number: 22CS30036
    ============================================================
*/

#ifndef HEADER_H
#define HEADER_H

/* HEADER FILES */
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

/* MACROS */
#define T 5
#define PROB_DROP 0.05
#define KEY 101
#define SEMKEY 102
#define MAXNKSOCK 10
#define MAXNMSG 10
#define MAXSEQNUM 255
#define MSGSIZE 512
#define DATAMSG 0
#define ACKMSG 1
#define QUERYMSG 2
#define SOCK_KTP 100
#define ENOSPACE 200
#define ENOTBOUND 201
#define ENOMESSAGE 202

// Global error variable
extern int errno;

/*
    Data structure for sending window
*/
struct swnd
{
    int size;         // size of the sending window at any point of time
    int seq[MAXNMSG]; // sequence number window
    int next;         // next sequence number to be used while sending a message
    time_t timer;     // timer for messages within [0..next-1]
};

/*
    Data structure to maintain sender message buffer
*/
struct swnd_buff
{
    int start;                   // index of message corresponding to oldest sequence number that is not yet acknowledged
    int end;                     // index where new messages will be stored
    char data[MAXNMSG][MSGSIZE]; // buffer for storing the messages
};

/*
    Data structure for receiving window
*/
struct rwnd
{
    int size;         // size of the receiving window at any point of time
    int seq[MAXNMSG]; // sequence number receiver is expecting to receive
};

/*
    Data structure to maintain receiver message buffer
*/
struct rwnd_buff
{
    int read_seq;                // sequence number of message to be read next
    int rcv_seq[MAXNMSG];        /*
                                     to store whether a slot is empty or occupied by some message in the buffer
                                     if empty slot[i] = -1;
                                     else     slot[i] = sequence number of the message in the buff[i];
                                 */
    char data[MAXNMSG][MSGSIZE]; // buffer for storing the receiver messages that are not yet read by the application
};

/*
    Data Structure to store information about each KTP socket
*/
struct ksock_info
{
    int isalloc;
    int pid;
    int udp_sockfd;
    int isbind;
    struct sockaddr_in src_addr;
    struct sockaddr_in dest_addr;
    struct swnd_buff send_buff;
    struct rwnd_buff rcv_buff;
    struct swnd send_info;
    struct rwnd rcv_info;
};

/*
    Data Structure to represent Header in Message
*/
struct KTPHeader
{
    int type;
    int seq_no;
    int ack_no;
    int rwnd_size;
    int ref_type;
};

/*
    Data Structure to represent a Message
*/
struct KTPMessage
{
    struct KTPHeader header;
    char data[MSGSIZE];
};

/* Function Prototypes */

int k_socket(int domain, int type, int protocol);
int k_bind(int ksockfd, const struct sockaddr_in src_addr, socklen_t src_addrlen,
           const struct sockaddr_in dest_addr, socklen_t dest_addrlen);
ssize_t k_sendto(int ksockfd, char buff[], size_t len, int flags,
                 const struct sockaddr_in dest_addr, socklen_t dest_addrlen);
ssize_t k_recvfrom(int ksockfd, char buff[], size_t len, int flags);
int k_close(int fd);
int dropMessage(float p);
const char *errmsg(int err);

/* Auxillary Function Prototypes */

// Return True if the sending buffer is Full corresponfing to ksockfd
int isFull(struct ksock_info *SM, int ksockfd);

// Return True if the sending buffer if empty corresponfing to ksockfd
int isempty(struct ksock_info *SM, int ksockfd);

#endif