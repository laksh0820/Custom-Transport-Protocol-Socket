#include "ksocket.h"

// Structure used to define operation on the semaphores in semaphore set
struct sembuf pop = {0, -1, 0}, vop = {0, 1, 0};

/* pop is the structure we pass for doing the P(s) operation */
#define P(s) semop(s, &pop, 1)

/* vop is the structure we pass for doing the V(s) operation */
#define V(s) semop(s, &vop, 1)

/*------------------------------- FUNCTION IMPLEMENTATION  ---------------------------------- */

int isFull(struct ksock_info *SM, int ksockfd)
{
    return SM[ksockfd].send_buff.start == (SM[ksockfd].send_buff.end + 1) % MAXNMSG;
}

int isempty(struct ksock_info *SM, int ksockfd)
{
    return SM[ksockfd].send_buff.start == -1 && SM[ksockfd].send_buff.end == -1;
}

const char *errmsg(int err)
{
    if (err == ENOSPACE)
    {
        return "No space available";
    }
    else if (err == ENOMESSAGE)
    {
        return "No message found";
    }
    else if (err == ENOTBOUND)
    {
        return "Not bounded";
    }
}

int k_socket(int domain, int type, int protocol)
{
    // Only SOCK_KTP is supported
    if (type != SOCK_KTP)
    {
        printf("Unsupported socket type\n");
        return -1;
    }

    // Shared Memory SM ID
    int shmid = shmget(KEY, sizeof(struct ksock_info) * MAXNKSOCK, 0777 | IPC_CREAT);
    struct ksock_info *SM = (struct ksock_info *)shmat(shmid, 0, 0);

    // Semaphore to ensure mutual exclusion
    int semid = semget(SEMKEY, 1, 0777 | IPC_CREAT);

    // checks for free space in SM
    for (int i = 0; i < MAXNKSOCK; ++i)
    {
        // Entry Section
        P(semid);

        // Critical section
        if (SM[i].isalloc == 0)
        {
            SM[i].isalloc = 1;
            SM[i].pid = getpid();
            SM[i].send_info.size = MAXNMSG;
            SM[i].rcv_info.size = MAXNMSG;
            SM[i].send_info.next = 0;
            for (int j = 0; j < MAXNMSG; ++j)
            {
                SM[i].send_info.seq[j] = j + 1;
                SM[i].rcv_info.seq[j] = j + 1;
                SM[i].rcv_buff.rcv_seq[j] = -1;
            }
            SM[i].send_buff.start = -1;
            SM[i].send_buff.end = -1;
            SM[i].rcv_buff.read_seq = 1;

            // Exit section
            V(semid);
            shmdt(SM);
            return i;
        }

        // Exit section
        V(semid);
    }

    // No free space is available
    errno = ENOSPACE;
    shmdt(SM);
    return -1;
}

int k_bind(int ksockfd, const struct sockaddr_in src_addr, socklen_t src_addrlen,
           const struct sockaddr_in dest_addr, socklen_t dest_addrlen)
{
    // Shared Memory SM ID
    int shmid = shmget(KEY, sizeof(struct ksock_info) * MAXNKSOCK, 0777 | IPC_CREAT);
    struct ksock_info *SM = (struct ksock_info *)shmat(shmid, 0, 0);

    // Semaphore to ensure mutual exclusion
    int semid = semget(SEMKEY, 1, 0777 | IPC_CREAT);

    // Entry Section
    P(semid);

    // Critical Section
    // Update the corresponding SM with src_addr and dest_addr
    SM[ksockfd].isbind = 1;
    SM[ksockfd].src_addr = src_addr;
    SM[ksockfd].dest_addr = dest_addr;

    // Exit section
    V(semid);

    shmdt(SM);
    return 0;
}

ssize_t k_sendto(int ksockfd, char buff[], size_t len, int flags,
                 const struct sockaddr_in dest_addr, socklen_t dest_addrlen)
{
    // Shared Memory SM ID
    int shmid = shmget(KEY, sizeof(struct ksock_info) * MAXNKSOCK, 0777 | IPC_CREAT);
    struct ksock_info *SM = (struct ksock_info *)shmat(shmid, 0, 0);

    // Semaphore to ensure mutual exclusion
    int semid = semget(SEMKEY, 1, 0777 | IPC_CREAT);

    // Entry Section
    P(semid);

    // Critical Section
    // Check if dest_ip and dest_port matches bounded_ip and bounded_port
    if (SM[ksockfd].dest_addr.sin_addr.s_addr == dest_addr.sin_addr.s_addr &&
        SM[ksockfd].dest_addr.sin_port == dest_addr.sin_port)
    {
        if (isFull(SM, ksockfd))
        {
            // If no space in send buffer return -1 and set the errno to ENOSPACE
            errno = ENOSPACE;

            // Exit section
            V(semid);
            shmdt(SM);
            return -1;
        }
        else
        {
            // write the message to the sender side message buffer
            int start = SM[ksockfd].send_buff.start;
            int end = SM[ksockfd].send_buff.end;
            if (isempty(SM, ksockfd))
            {
                // Sending buffer is empty
                start = 0;
                end = 0;
            }
            else
            {
                end = (end + 1) % MAXNMSG;
            }
            strncpy(SM[ksockfd].send_buff.data[end], buff, len);
            SM[ksockfd].send_buff.start = start;
            SM[ksockfd].send_buff.end = end;

#ifdef DEBUG
            printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
            printf("Done writing the data [%s] to sender buffer IP:[%s] PORT:[%d]\n", buff,
                   inet_ntoa(SM[ksockfd].src_addr.sin_addr), ntohs(SM[ksockfd].src_addr.sin_port));
            printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
#endif

            // Exit section
            V(semid);
            shmdt(SM);
            return len;
        }
    }
    else
    {
        // Drop the message
        errno = ENOTBOUND;
    }

    // Exit section
    V(semid);
    shmdt(SM);
    return -1;
}

ssize_t k_recvfrom(int ksockfd, char buff[], size_t len, int flags)
{
    // Shared Memory SM ID
    int shmid = shmget(KEY, sizeof(struct ksock_info) * MAXNKSOCK, 0777 | IPC_CREAT);
    struct ksock_info *SM = (struct ksock_info *)shmat(shmid, 0, 0);

    // Semaphore to ensure mutual exclusion
    int semid = semget(SEMKEY, 1, 0777 | IPC_CREAT);

#ifdef DEBUG
    printf("Next Read Sequence Number %d\n", SM[ksockfd].rcv_buff.read_seq);
#endif

    while (1)
    {
        int isread = 0;
        for (int i = 0; i < MAXNMSG; i++)
        {
            // Entry Section
            P(semid);

            // Critical section
            if (SM[ksockfd].rcv_buff.rcv_seq[i] == SM[ksockfd].rcv_buff.read_seq)
            {
                strcpy(buff, SM[ksockfd].rcv_buff.data[i]);
                SM[ksockfd].rcv_buff.rcv_seq[i] = -1;
                SM[ksockfd].rcv_buff.read_seq += 1;
                SM[ksockfd].rcv_buff.read_seq %= (MAXSEQNUM + 1);
                SM[ksockfd].rcv_info.size += 1;

#ifdef DEBUG
                printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
                printf("Done reading the data [%s] from receiver buffer IP:[%s] PORT:[%d]\n", SM[ksockfd].rcv_buff.data[i],
                       inet_ntoa(SM[ksockfd].src_addr.sin_addr), ntohs(SM[ksockfd].src_addr.sin_port));
                printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
#endif

                // Exit section
                V(semid);
                isread = 1;
                break;
            }

            // Exit section
            V(semid);
        }

        if (isread)
        {
            shmdt(SM);
            return len;
        }
    }

    errno = ENOMESSAGE;
    shmdt(SM);
    return -1;
}

int k_close(int fd)
{
    // Shared Memory SM ID
    int shmid = shmget(KEY, sizeof(struct ksock_info) * MAXNKSOCK, 0777 | IPC_CREAT);
    struct ksock_info *SM = (struct ksock_info *)shmat(shmid, 0, 0);

    // Semaphore to ensure mutual exclusion
    int semid = semget(SEMKEY, 1, 0777 | IPC_CREAT);

    // Entry Section
    P(semid);

    // Critical section
    // Clean up socket entry in the SM if there is no further data to send
    if (isempty(SM, fd))
    {
        SM[fd].isalloc = 0;
        SM[fd].pid = -1;
        SM[fd].isbind = 0;
    }

    // Exit section
    V(semid);
    shmdt(SM);
    return 0;
}

/*-------------------------------------------------------------------------------------------- */
