#include "ksocket.h"

int main()
{
    struct sockaddr_in usr_addr, dest_addr;

    int M2 = k_socket(AF_INET, SOCK_KTP, 0);
    if (M2 == -1)
    {
        printf("Unable to create a KTP socket\n");
        exit(1);
    }
    printf("ksockfd : %d\n", M2);

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

    if (k_bind(M2, usr_addr, sizeof(usr_addr), dest_addr, sizeof(dest_addr)) == -1)
    {
        printf("Unable to bind the KTP socket\n");
        k_close(M2);
        exit(2);
    }

    char filename[1024];
    printf("Enter the filename : ");
    scanf("%s",filename);
    FILE* fp = fopen(filename,"w");
    if (fp == NULL)
    {
        printf("Unable to open a file\n");
        fclose(fp);
        k_close(M2);
        exit(3);
    }

    while (1)
    {
        char buff[MSGSIZE];
        for (int i = 0; i < MSGSIZE; ++i)
        {
            buff[i] = '\0';
        }
        if (k_recvfrom(M2, buff, sizeof(buff), 0) == -1)
        {
            continue;
        }
        int done = 0;
        for (int i = 0; i < MSGSIZE; ++i)
        {
            if (buff[i] == '$')
            {
                done = 1;
                break;
            }
            if (buff[i] == '\0')
            {
                break;
            }
            fprintf(fp, "%c", buff[i]);
        }
        if (done)
            break;
    }

    fclose(fp);
    k_close(M2);
    return 0;
}
