/*
    ============================================================
    Assignment 4 Submission
    Name: Lakshya Agrawal
    Roll number: 22CS30036
    ============================================================
*/

#include "ksocket.h"

int main()
{
    struct sockaddr_in usr_addr, dest_addr;

    int M1 = k_socket(AF_INET, SOCK_KTP, 0);
    if (M1 == -1)
    {
        printf("Unable to create a KTP socket\n");
        exit(1);
    }
    printf("ksockfd : %d\n", M1);

    int src_port,dest_port;

    usr_addr.sin_family = AF_INET;
    usr_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    printf("Enter source port : ");
    scanf("%d",&src_port);
    usr_addr.sin_port = htons(src_port);

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    printf("Enter destination port : ");
    scanf("%d",&dest_port);
    dest_addr.sin_port = htons(dest_port);

    if (k_bind(M1, usr_addr, sizeof(usr_addr), dest_addr, sizeof(dest_addr)) == -1)
    {
        printf("Unable to bind the KTP socket\n");
        k_close(M1);
        exit(2);
    }

    char filename[1024];
    printf("Enter the filename : ");
    scanf("%s",filename);
    FILE* fp = fopen(filename,"r");
    if (fp == NULL)
    {
        printf("Unable to open a file\n");
        fclose(fp);
        k_close(M1);
        exit(3);
    }

    while (1)
    {
        char c;
        char buff[MSGSIZE];
        for (int i = 0; i < MSGSIZE; ++i)
        {
            buff[i] = '\0';
        }
        int done = 0;
        int i;
        for (i = 0; i < MSGSIZE - 1; ++i)
        {
            int isEOF = (fscanf(fp, "%c", &c) == EOF);
            if (isEOF)
            {
                done = 1;
                buff[i] = '$';
                i++;
                break;
            }
            buff[i] = c;
        }
        buff[i] = '\0';
        while (k_sendto(M1, buff, strlen(buff) + 1, 0, dest_addr, sizeof(dest_addr)) == -1)
        {
            sleep(10);
            printf("Unable to send the message : %s\n", errmsg(errno));
        }
        if (done)
            break;
    }

    fclose(fp);
    k_close(M1);
    return 0;
}