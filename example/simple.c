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

#include "ff_config.h"
#include "ff_api.h"

int sockfd;

/* receiver
int loop(void *arg)
{
    char buf[1024] = {0};
    int nb = ff_read(sockfd, buf, 1024);
    if(nb <= 0) return 0;
    printf("Read: %s\n", buf);

    sprintf(buf, "Hello, World\n");
    ff_write(sockfd, buf, strlen(buf));
    ff_write(sockfd, buf, strlen(buf));
    //ff_close(sockfd);
    return 1;
}*/

struct sockaddr_in clientaddr; /* client addr */

#define BUFSIZE 1024
int loop(void *arg)
{
    int clientlen = sizeof(clientaddr);
    char buf[BUFSIZ] = {0};
    int nb = ff_recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr*)&clientaddr, &clientlen);
    if(nb <= 0) return 0;
    printf("Read: %s\n", buf);


    sprintf(buf, "Hello, World\n");
    ff_sendto(sockfd, buf, strlen(buf), 0, 
	       (struct sockaddr *) &clientaddr, clientlen);
    //close(sockfd);
    return 0;
}

int main(int argc, char * argv[])
{
    ff_init(argc, argv);

    sockfd = ff_socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("ff_socket failed, sockfd:%d, errno:%d, %s\n", sockfd, errno, strerror(errno));
        exit(1);
    }

    struct sockaddr_in my_addr;
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(1234);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int ret = ff_bind(sockfd, (struct linux_sockaddr *)&my_addr, sizeof(my_addr));
    if (ret < 0) {
        printf("ff_bind failed, sockfd:%d, errno:%d, %s\n", sockfd, errno, strerror(errno));
        exit(1);
    }

    ff_run(loop, NULL);
    return 0;
}
