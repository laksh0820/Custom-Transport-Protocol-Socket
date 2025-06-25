/*
    ============================================================
    Assignment 4 Submission
    Name: Lakshya Agrawal
    Roll number: 22CS30036
    ============================================================
*/

#include "ksocket.h"

// Global Variables

// Count the number of Transmissions
int Transmission = 0;

// Semaphore for ensuring mutual exculsion
int semid;

// Structure used to define operation on the semaphores in semaphore set
struct sembuf pop, vop;

// ID of the shared memory
int shmid;

// To store nospace flag if the reciving side buffer is full
int nospace[MAXNKSOCK];

// To access shared memory
struct ksock_info *SM;

/* pop is the structure we pass for doing the P(s) operation */
#define P(s) semop(s, &pop, 1)

/* vop is the structure we pass for doing the V(s) operation */
#define V(s) semop(s, &vop, 1)

// Function to return max
int max(int a, int b)
{
    return ((a > b) ? a : b);
}

// Function to drop a message
int dropMessage(float p)
{
    double rand_num = (double)rand() / (double)((unsigned)RAND_MAX + 1);

    // #ifdef DEBUG
    // printf("Random Number inside dropMessage : %.4f\n",rand_num);
    // #endif

    return rand_num < p;
}

/*----------------------------------------- HELPER FUNCTIONS ---------------------------------------------*/

// Return True if the sending buffer is Full corresponfing to ksockfd
int isFull(struct ksock_info *SM, int ksockfd)
{
    return SM[ksockfd].send_buff.start == (SM[ksockfd].send_buff.end + 1) % MAXNMSG;
}

// Return True if the sending buffer if empty corresponfing to ksockfd
int isempty(struct ksock_info *SM, int ksockfd)
{
    return SM[ksockfd].send_buff.start == -1 && SM[ksockfd].send_buff.end == -1;
}

// Function to send a message
void send_msg(int ksockfd, int type, int seq_no, int ack_no, int rwnd_size, int ref_type, char data[])
{
    // call dropMessage
    if (dropMessage(PROB_DROP))
    {
#ifdef DEBUG
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        if (type == DATAMSG)
        {
            printf("Data ");
            Transmission++;
        }
        else if (type == ACKMSG)
        {
            printf("ACK ");
        }
        else
        {
            printf("QUERY ");
        }
        printf("Message Dropped while sending from IP[%s] PORT[%d]\n", inet_ntoa(SM[ksockfd].src_addr.sin_addr),
               ntohs(SM[ksockfd].src_addr.sin_port));
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
#endif
        return;
    }

    struct KTPMessage msg;
    msg.header.type = type;
    msg.header.seq_no = seq_no;
    msg.header.ack_no = ack_no;
    msg.header.rwnd_size = rwnd_size;
    msg.header.ref_type = ref_type;
    strcpy(msg.data, data);
    sendto(SM[ksockfd].udp_sockfd, &msg, sizeof(msg), 0,
           (struct sockaddr *)&(SM[ksockfd].dest_addr), sizeof(SM[ksockfd].dest_addr));

#ifdef DEBUG
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    if (type == DATAMSG)
    {
        printf("Data Message SEQ NO [%d] SEND_INFO_NEXT[%d]", seq_no, SM[ksockfd].send_info.next);
        Transmission++;
    }
    else if (type == ACKMSG)
    {
        printf("ACK Message SEQ NO [%d]", ack_no);
    }
    else
    {
        printf("QUERY Message");
    }
    printf(" [%s] send to IP[%s] PORT[%d]\n", data, inet_ntoa(SM[ksockfd].dest_addr.sin_addr),
           ntohs(SM[ksockfd].dest_addr.sin_port));
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
#endif
}

// Function to update the receiving side window
void rwnd_slide(int ksockfd, int end)
{
    for (int i = 0; i < MAXNMSG; ++i)
    {
        SM[ksockfd].rcv_info.seq[i] += (end + 1);
        SM[ksockfd].rcv_info.seq[i] %= (MAXSEQNUM + 1);
    }
}

// Function to update the sending side window
void swnd_slide(int ksockfd, int end)
{
    for (int i = 0; i < MAXNMSG; ++i)
    {
        SM[ksockfd].send_info.seq[i] += (end + 1);
        SM[ksockfd].send_info.seq[i] %= (MAXSEQNUM + 1);
    }
}

// Function to return the free slot in the receving-message buffer
int getfreeslot(int rcv_seq[])
{
    for (int i = 0; i < MAXNMSG; ++i)
    {
        if (rcv_seq[i] == -1)
        {
            return i;
        }
    }
    return -1;
}

// Function to return the last in-order acknowledged message in the receiver window
int getlastinorderidx(int rcv_seq[], int seq[])
{
    int idx = MAXNMSG - 1;
    for (int i = 0; i < MAXNMSG; ++i)
    {
        int present = 0;
        for (int j = 0; j < MAXNMSG; ++j)
        {
            if (seq[i] == rcv_seq[j])
            {
                present = 1;
                break;
            }
        }
        if (present == 0)
        {
            idx = i - 1;
            break;
        }
    }
    return idx;
}

// Handle UDP socket inside Ksocket
void handleKsocket(int i)
{
    if (SM[i].udp_sockfd != -1 && SM[i].isbind == 0)
    {
        // close the UDP socket
        close(SM[i].udp_sockfd);
        SM[i].udp_sockfd = -1;
    }

    if (SM[i].udp_sockfd == -1)
    {
        // Create a UDP socket
        SM[i].udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    }

    if (SM[i].isbind == 1)
    {
        // Bind the socket to the source IP and source PORT
        SM[i].isbind = 2;
        bind(SM[i].udp_sockfd, (struct sockaddr *)&(SM[i].src_addr), sizeof(SM[i].src_addr));
    }
}

/*--------------------------------------------------------------------------------------------------------*/

/*----------------------------------------- GARBAGE CLEANER ---------------------------------------------*/

// Check whether a process is running or not
int is_process_running(int pid)
{
    if (kill(pid, 0) == 0)
    {
        // Process is still running
        return 1;
    }
    else
    {
        if (errno == ESRCH)
        {
            // Process does not exit
            return 0;
        }
        else
        {
            // Error Occurred
            perror("kill");
            return -1;
        }
    }
}

// Clean the ksocket
void cleanup_ksocket(int fd)
{
    // Clean up socket entry in the SM
    SM[fd].isalloc = 0;
    SM[fd].pid = -1;
    SM[fd].isbind = 0;
    close(SM[fd].udp_sockfd);
    SM[fd].udp_sockfd = -1;
}

// Thread to cleanup the kscoket (if process exited without closing the socket)
void garbage_collector()
{
    for (int i = 0; i < MAXNKSOCK; ++i)
    {
        // Entry Section
        P(semid);

        // Critical Section
        if (SM[i].isalloc)
        {
            // Active socket
            if (!is_process_running(SM[i].pid) && isempty(SM, i))
            {
                // Process closed and no data left in the sending buffer
                cleanup_ksocket(i);
            }
        }

        // Exit Section
        V(semid);
    }
}

/*-------------------------------------------------------------------------------------------------------*/

/*----------------------------------------- THREAD R -----------------------------------------------------*/

// Thread R handles all messages received from the UDP socket
void *thread_R()
{
    fd_set rcv_fds;

    while (1)
    {
        // Clean the garbage (process exited without properly closing the socket)
        garbage_collector();

        // initialize the receving file descriptors
        FD_ZERO(&rcv_fds);
        for (int i = 0; i < MAXNKSOCK; i++)
        {
            // Entry Section
            P(semid);

            // Critical section
            if (SM[i].isalloc)
            {
                handleKsocket(i);

                if (SM[i].isbind == 2)
                {
                    FD_SET(SM[i].udp_sockfd, &rcv_fds);
                }
            }

            // Exit Section
            V(semid);
        }

        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        int retval = select(FD_SETSIZE, &rcv_fds, NULL, NULL, &tv);

        if (retval == -1)
        {
            perror("select failed");
        }
        else if (retval == 0)
        {
            // Timeout

            // checks if "nospace" flag was set
            for (int i = 0; i < MAXNKSOCK; i++)
            {
                P(semid);

                // Critical section
                if (SM[i].isalloc)
                {
                    if (nospace[i] && SM[i].rcv_info.size != 0)
                    {
                        // reset flag
                        nospace[i] = 0;

                        // send acknowledgement containg the updated rwnd size
                        send_msg(i, ACKMSG, 0, (SM[i].rcv_info.seq[0] - 1 + MAXSEQNUM + 1) % (MAXSEQNUM + 1),
                                 SM[i].rcv_info.size, QUERYMSG, "");
                    }
                }

                V(semid);
            }
        }
        else
        {
            // Findout to which KTP socket(s) the message has arrived
            for (int i = 0; i < MAXNKSOCK; i++)
            {
                if (!SM[i].isalloc)
                    continue;

                // Entry Section
                P(semid);

                // Critical Section
                if (SM[i].udp_sockfd != -1 && FD_ISSET(SM[i].udp_sockfd, &rcv_fds))
                {
                    // Process the message packet
                    struct KTPMessage msg;
                    int bytes_read = recvfrom(SM[i].udp_sockfd, &msg, sizeof(msg), 0, NULL, NULL);

#ifdef DEBUG
                    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
                    if (msg.header.type == DATAMSG)
                    {
                        printf("Data Message SEQ NO [%d]", msg.header.seq_no);
                    }
                    else if (msg.header.type == ACKMSG)
                    {
                        printf("ACK Message SEQ NO [%d] RWND SIZE [%d]", msg.header.ack_no, msg.header.rwnd_size);
                    }
                    else
                    {
                        printf("QUERY Message");
                    }
                    printf(" received on IP[%s] PORT[%d]\n", inet_ntoa(SM[i].src_addr.sin_addr),
                           ntohs(SM[i].src_addr.sin_port));
                    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
#endif

                    if (bytes_read > 0)
                    {
                        if (msg.header.type == DATAMSG)
                        {
                            // Data Packet

                            int start = (SM[i].rcv_info.seq[0] - MAXNMSG + MAXSEQNUM + 1) % (MAXSEQNUM + 1);
                            int end = (SM[i].rcv_info.seq[0] - 1 + MAXSEQNUM + 1) % (MAXSEQNUM + 1);

                            if (msg.header.seq_no >= start && msg.header.seq_no <= end)
                            {
                                // Region 1
                                // Send an ACK message to the sender
                                send_msg(i, ACKMSG, 0, end, SM[i].rcv_info.size, 0, "");
                            }
                            else
                            {
                                start = SM[i].rcv_info.seq[0];
                                end = SM[i].rcv_info.seq[MAXNMSG - 1];

                                if (msg.header.seq_no >= start && msg.header.seq_no <= end)
                                {
                                    // Region 2

                                    // Check for duplicates
                                    int isdup = 0;
                                    for (int j = 0; j < MAXNMSG; j++)
                                    {
                                        if (msg.header.seq_no == SM[i].rcv_buff.rcv_seq[j])
                                        {
                                            isdup = 1;
                                            break;
                                        }
                                    }
                                    if (isdup)
                                    {
                                        V(semid);
                                        continue;
                                    }

                                    // Store the data in receiver-side message buffer
                                    int freeslot = getfreeslot(SM[i].rcv_buff.rcv_seq);
                                    strcpy(SM[i].rcv_buff.data[freeslot], msg.data);
                                    SM[i].rcv_buff.rcv_seq[freeslot] = msg.header.seq_no;
                                    SM[i].rcv_info.size--;

                                    // update the receiving window information
                                    int ackidx = getlastinorderidx(SM[i].rcv_buff.rcv_seq, SM[i].rcv_info.seq);
                                    if (ackidx == -1)
                                    {
                                        // Do nothing
                                    }
                                    else
                                    {
                                        send_msg(i, ACKMSG, 0, SM[i].rcv_info.seq[ackidx], SM[i].rcv_info.size, 0, "");
                                        rwnd_slide(i, ackidx);
                                    }
                                }
                                else
                                {
                                    // Do nothing
                                }
                            }
                        }
                        else if (msg.header.type == ACKMSG)
                        {
                            // Acknowledgement Packet
                            int ackidx = -1;
                            for (int j = 0; j < SM[i].send_info.next; j++)
                            {
                                if (SM[i].send_info.seq[j] == msg.header.ack_no)
                                {
                                    ackidx = j;
                                    break;
                                }
                            }

                            if (ackidx != -1)
                            {
                                swnd_slide(i, ackidx);
                                SM[i].send_info.next -= (ackidx + 1);
                                if ((SM[i].send_buff.start + ackidx + 1) % MAXNMSG == (SM[i].send_buff.end + 1) % MAXNMSG)
                                {
                                    SM[i].send_buff.start = SM[i].send_buff.end = -1;
                                }
                                else
                                {
                                    SM[i].send_buff.start += ackidx + 1;
                                    SM[i].send_buff.start %= MAXNMSG;
                                }
                                SM[i].send_info.size = msg.header.rwnd_size;

                                if (SM[i].send_info.next != 0)
                                {
                                    SM[i].send_info.timer = time(0);
                                }
                            }
                            else
                            {
                                if (msg.header.ref_type == QUERYMSG)
                                {
                                    // rwnd size update message
                                    SM[i].send_info.size = msg.header.rwnd_size;
                                }
                            }

#ifdef DEBUG
                            printf("SENDER START[%d] END[%d]\n\n", SM[i].send_buff.start, SM[i].send_buff.end);
#endif
                        }
                        else
                        {
                            // Query Packet

                            // Send a acknowledgement packet if the rwnd.size != 0
                            if (SM[i].rcv_info.size != 0)
                            {
                                send_msg(i, ACKMSG, 0, (SM[i].rcv_info.seq[0] - 1 + MAXSEQNUM + 1) % (MAXSEQNUM + 1),
                                         SM[i].rcv_info.size, QUERYMSG, "");
                            }
                        }
                    }
                    else if (bytes_read < 0)
                    {
                        perror("recvfrom inside Thread R failed");
                    }

                    // set the flag "nospace"
                    if (SM[i].rcv_info.size == 0)
                    {
                        nospace[i] = 1;
                    }
                }

                // Exit Section
                V(semid);
            }
        }
    }
}

/*-------------------------------------------------------------------------------------------------------*/

/*----------------------------------------- THREAD S ----------------------------------------------------*/

// Thread S handles timeouts and retransmissions
void *thread_S()
{
    while (1)
    {
        // sleep for some time
        sleep(T / 2);

        // Clean the garbage (process exited without properly closing the socket)
        garbage_collector();

        for (int i = 0; i < MAXNKSOCK; ++i)
        {
            P(semid);

            // Critical section
            if (SM[i].isalloc)
            {
                // Active ksocket

                handleKsocket(i);

                // check whether the message timeout period is over or not
                if (SM[i].isbind == 2 && time(0) - SM[i].send_info.timer > T)
                {
                    // Timeout

                    SM[i].send_info.timer = time(0);
                    if (SM[i].send_info.size == 0)
                    {
                        // Sending window is in stopped state as receiving buffer is full
                        send_msg(i, QUERYMSG, 0, 0, 0, 0, "");
                    }
                    else
                    {
                        // Retransmit all the messages within the current window and pending message in the sending buffer
                        char data[MSGSIZE];
                        for (int j = SM[i].send_buff.start, k = 0; k < SM[i].send_info.size; ++j, j %= MAXNMSG, ++k)
                        {
                            strcpy(data, SM[i].send_buff.data[j]);
                            send_msg(i, DATAMSG, SM[i].send_info.seq[k], 0, 0, 0, data);
                            if (k == SM[i].send_info.next)
                            {
                                SM[i].send_info.next = k + 1;
                            }
                            if (j == SM[i].send_buff.end)
                                break;
                        }
                    }
                }
            }

            V(semid);
        }
    }
}

/*-------------------------------------------------------------------------------------------------------*/

int main()
{
    // Initialize the seed for random number generation
    srand(time(NULL));

    pthread_t R, S;

    // initialize nospace flag
    for (int i = 0; i < MAXNKSOCK; i++)
    {
        nospace[i] = 0;
    }

    // Create the semaphore set with 1 semaphore
    if ((semid = semget(SEMKEY, 1, 0777 | IPC_CREAT)) == -1)
    {
        perror("semget");
        exit(1);
    }

    // Initialize the semaphore to 1 (unlocked)
    if (semctl(semid, 0, SETVAL, 1) == -1)
    {
        perror("semctl");
        exit(1);
    }

    // Initialize the sembufs pop and vop
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1;
    vop.sem_op = 1;

    // Creating a shared memory
    shmid = shmget(KEY, sizeof(struct ksock_info) * MAXNKSOCK, 0777 | IPC_CREAT);

    if (shmid == -1)
    {
        perror("Unable to create shared memory");
        exit(1);
    }

    // Attach to the shared memory
    SM = (struct ksock_info *)shmat(shmid, 0, 0);

    // Initialize the Shared Memory
    for (int i = 0; i < MAXNKSOCK; ++i)
    {
        // Entry Section
        P(semid);

        // Critical Section
        SM[i].isalloc = 0;
        SM[i].pid = -1;
        SM[i].udp_sockfd = -1;
        SM[i].isbind = 0;

        // Exit Section
        V(semid);
    }

    // Creating the threads
    pthread_create(&R, NULL, thread_R, NULL);
    pthread_create(&S, NULL, thread_S, NULL);

    // Display a welcome message
    printf("initksocket is running...\n");

    // To close the initksocket process
    printf("Enter \"close\" to close the init process\n");
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    int retval = select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
    if (retval == -1)
    {
        perror("select()");
    }
    else
    {
        if (FD_ISSET(STDIN_FILENO, &rfds))
        {
            char cmd[10];
            int nread = read(0, cmd, sizeof(cmd));
            cmd[nread - 1] = '\0';
            if (!strcmp(cmd, "close"))
            {
                pthread_cancel(R);
                pthread_cancel(S);
            }
        }
    }

#ifdef DEBUG
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("Total Number of Transmissions [%d]\n", Transmission);
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
#endif

    // Remove the semaphore
    semctl(semid, 0, IPC_RMID, 0);

    // Detach from the shared memory
    shmdt(SM);

    // Delete the created memory
    shmctl(shmid, IPC_RMID, 0);

    return 0;
}