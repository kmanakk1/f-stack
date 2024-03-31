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
#include <unistd.h>

#include "ff_config.h"
#include "ff_api.h"

int sockfd;

int loop(void *arg)
{
    char buf[2048];
    char w[] = "GET / HTTP/1.1\r\n\r\n";
    /* Wait for events to happen */
    ssize_t writelen = ff_write(sockfd, w, strlen(w));
    ssize_t readlen = ff_read(sockfd, buf, 2047);

    if(readlen > 0) {
        printf("%d, %s\n", readlen, buf);
    }
    return 0;
}

int main(int argc, char * argv[])
{
    ff_init(argc, argv);

    sockfd = ff_socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("ff_socket failed, sockfd:%d, errno:%d, %s\n", sockfd, errno, strerror(errno));
        exit(1);
    }

    struct sockaddr_in my_addr;
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(80);
    my_addr.sin_addr.s_addr = htonl(inet_addr("10.1.1.2"));

    sleep(3);
    int ret = ff_connect(sockfd, (struct linux_sockaddr *)&my_addr, sizeof(my_addr));
    if (ret < 0) {
        printf("ff_connect failed, sockfd:%d, errno:%d, %s - %d\n", sockfd, errno, strerror(errno), ret);
        exit(1);
    }

    ff_run(loop, NULL);
    return 0;
}
