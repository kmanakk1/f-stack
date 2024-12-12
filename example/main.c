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
#include <netinet/tcp.h>


#include "ff_config.h"
#include "ff_api.h"

#define MAX_EVENTS 512

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
char buf[256* 1000];

char html[] =
"HTTP/1.0 200 bu OK\r\n"
"Server: F-Stack\r\n"
"Date: Sat, 25 Feb 2017 09:26:33 GMT\r\n"
"Content-Type: text/plain\r\n"
"Content-Length: 64\r\n"
"Connection: keep-alive\r\n"
"\r\n";
char body[] =
"88888888888888888888888888888888888888888888888888888888888888\r\n";

long int pktcount = 0;

int loop(void *arg)
{
    /* Wait for events to happen */
    int nevents = ff_kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);
    int i;
    static int recv_bytes = 0;
    static struct timeval time_ahead, time_after;

    if (nevents < 0) {
        printf("ff_kevent failed:%d, %s\n", errno,
                        strerror(errno));
        return -1;
    }
    static struct timeval delay_past, delay_now;
    for (i = 0; i < nevents; ++i) {
        
        struct kevent event = events[i];
        int clientfd = (int)event.ident;
        // gettimeofday(&delay_now, NULL);
        // printf("delay:%f",(delay_now.tv_sec - delay_past.tv_sec) + (delay_now.tv_usec - delay_past.tv_usec)/1000000.0);
        // delay_past.tv_sec = delay_now.tv_sec;
        // delay_past.tv_usec = delay_now.tv_usec;
        /* Handle disconnect */
        if (event.flags & EV_EOF) {
            /* Simply close socket */
            ff_close(clientfd);

        } else if (clientfd == sockfd) {
            int available = (int)event.data;
            do {
                int nclientfd = ff_accept(clientfd, NULL, NULL);
                if (nclientfd < 0) {
                    printf("ff_accept failed:%d, %s\n", errno,
                        strerror(errno));
                    break;
                }

                /* Add to event list */
                EV_SET(&kevSet, nclientfd, EVFILT_READ, EV_ADD, 0, 0, NULL);

                if(ff_kevent(kq, &kevSet, 1, NULL, 0, NULL) < 0) {
                    printf("ff_kevent error:%d, %s\n", errno,
                        strerror(errno));
                    return -1;
                }

                available--;
            } while (available);
        } else if (event.filter == EVFILT_READ) {
            
            ssize_t readlen = ff_read(clientfd, buf, sizeof(buf));
            if(readlen <= 0)
            {
                continue;
            }
            pktcount++;
            // printf("len:%ld\n",readlen);
            /* get start time */
            if(recv_bytes == 0)
            {
                gettimeofday(&time_ahead, NULL);
            }
            recv_bytes += readlen;
            if(recv_bytes > 1000000 * 1000 * 10)
            {
                /*calculate speed*/
                gettimeofday(&time_after, NULL);
                double delta_time = (time_after.tv_sec - time_ahead.tv_sec) + (time_after.tv_usec - time_ahead.tv_usec)/1000000.0;
                double speed = (recv_bytes/1000000.0) / delta_time;
                printf("speed:%.6lfMB/s\n", speed);
                
                /* reset recvbytes */
                recv_bytes = 0;
            }
            //http reply
            // int ret = ff_write(clientfd,(const void *) html, strlen(html));
            // ret = ff_write(clientfd,(const void *) body, strlen(body));
            // ff_close(clientfd);

            if (readlen == -1)
            {
                ff_close(clientfd);
                printf("Connection down\n");
            }
            // printf("read length:%d\n", readlen);
            // printf("%s\n",buf);
        } else {
            printf("unknown event: %8.8X\n", event.flags);
        }
    }

    return 0;
}

void handle_alarm(int sig)
{
    (void)sig;
    printf("pps:%ld\n", pktcount/5);
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

    sockfd = ff_socket(AF_INET, SOCK_STREAM, 0);
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
    int max_meg = 1500;
    ret = ff_setsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &max_meg, sizeof(max_meg));

    ret = ff_listen(sockfd, MAX_EVENTS);
    if (ret < 0) {
        printf("ff_listen failed, sockfd:%d, errno:%d, %s\n", sockfd, errno, strerror(errno));
        exit(1);
    }

    EV_SET(&kevSet, sockfd, EVFILT_READ, EV_ADD, 0, MAX_EVENTS, NULL);
    /* Update kqueue */
    ff_kevent(kq, &kevSet, 1, NULL, 0, NULL);

    socklen_t optlen;
    int rcvbuf_actual, sndbuf_actual, mss_actual;
    optlen = sizeof(rcvbuf_actual);
    ff_getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf_actual, &optlen);
    ff_getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf_actual, &optlen);
    ff_getsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &mss_actual, &optlen);
    printf("RCVBUF:%d\nSNDBUF:%d\nMSS:%d\n", rcvbuf_actual, sndbuf_actual, mss_actual);

    signal(SIGALRM, handle_alarm);
    alarm(5);
    ff_run(loop, NULL);
    return 0;
}
