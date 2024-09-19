#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <signal.h>

#include "ff_config.h"
#include "ff_api.h"

#define MAX_EVENTS 102400

/* kevent set */
struct kevent kevSet;
/* events */
struct kevent events[MAX_EVENTS];
/* kq */
int kq;
int sockfd;
#ifdef INET6
int sockfd6;
#endif

char html[] =
"HTTP/1.1 200 OK\r\n"
"Server: F-Stack\r\n"
"Date: Sat, 25 Feb 2017 09:26:33 GMT\r\n"
"Content-Type: text/html\r\n"
"Content-Length: 438\r\n";
long int pktcount = 0;
long int loopcnt = 0;
long int empty_loop = 0;
int loop(void *arg)
{
    /* Wait for events to happen */
    int nevents = ff_kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);
    
    int i;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    static long long int recv_bytes = 0;
    static struct timeval time_ahead, time_after;

    loopcnt++;
    if(nevents == 0)
    {
        empty_loop ++;
    }
    // if(loopcnt == 1000)
    // {
    //     printf("%.4f\n", 100.0 * (empty_loop* 1.0)/loopcnt);
    //     loopcnt = 0;
    //     empty_loop = 0;
    // }
    if (nevents < 0) {
        printf("ff_kevent failed:%d, %s\n", errno,
                        strerror(errno));
        return -1;
    }


    for (i = 0; i < nevents; ++i) {
        
        struct kevent event = events[i];
        int clientfd = (int)event.ident;

        /* Handle disconnect */
        if (event.flags & EV_EOF) {
            /* Simply close socket */
            ff_close(clientfd);

        }  else if (event.filter == EVFILT_READ) {
            pktcount ++;
            static char buf[3000];
            ssize_t readlen = ff_recvfrom(clientfd, buf, sizeof(buf), 0, (struct linux_sockaddr*)&client_addr, &addr_len); 
            /* get start time */
            if(recv_bytes == 0)
            {
                gettimeofday(&time_ahead, NULL);
            }
            recv_bytes+= readlen;
            if(recv_bytes > 1000 * 1000 * 1000)
            {
                /*calculate speed*/
                gettimeofday(&time_after, NULL);
                double delta_time = (time_after.tv_sec - time_ahead.tv_sec) + (time_after.tv_usec - time_ahead.tv_usec)/(1000000.0);
                double speed = (8 * recv_bytes/(1024 * 1024.0)) / delta_time;
                printf("speed:%.6lfMbps\n", speed);
                /* reset recvbytes */
                recv_bytes = 0;
            }
            // printf("recv:%s\n", buf);
            // printf("recv:%ld\n", readlen);
        } else {
            printf("unknown event: %8.8X\n", event.flags);
        }
    }

    return 0;
}

void handle_alarm(int sig)
{
    (void)sig;
    printf("%ld\n", pktcount/5);
    pktcount= 0;
    alarm(5);
}

int main(int argc, char * argv[])
{
    ff_init(argc, argv);

    kq = ff_kqueue();
    if (kq < 0) {
        printf("ff_kqueue failed, errno:%d, %s\n", errno, strerror(errno));
        exit(1);
    }

    sockfd = ff_socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("ff_socket failed, sockfd:%d, errno:%d, %s\n", sockfd, errno, strerror(errno));
        exit(1);
    }

    /* Set non blocking */
    int on = 1;
    ff_ioctl(sockfd, FIONBIO, &on);

    struct sockaddr_in my_addr;
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(8080);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int ret = ff_bind(sockfd, (struct linux_sockaddr *)&my_addr, sizeof(my_addr));
    if (ret < 0) {
        printf("ff_bind failed, sockfd:%d, errno:%d, %s\n", sockfd, errno, strerror(errno));
        exit(1);
    }

    EV_SET(&kevSet, sockfd, EVFILT_READ, EV_ADD, 0, MAX_EVENTS, NULL);
    /* Update kqueue */
    ff_kevent(kq, &kevSet, 1, NULL, 0, NULL);

    int rcvbuf;
    socklen_t optlen = sizeof(rcvbuf);
    int buf_set = 4208000;
    socklen_t bufset_len = sizeof(buf_set);
    if (ff_setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buf_set, bufset_len) == 0) {
        printf("\nReceive buffer set size: %d bytes\n", buf_set);
    }
    else
        printf("set failed\n");
    if (ff_getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &optlen) == 0) {
        printf("\nReceive buffer size: %d bytes\n", rcvbuf);
    }
    else
        printf("query failed\n");

    signal(SIGALRM, handle_alarm);
    alarm(5);
    ff_run(loop, NULL);
    
    return 0;
}
