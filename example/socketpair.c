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

int fds[2];

int x = 0;
int loop(void *arg)
{
    if(x % 2 == 0) {
        ff_write(fds[0], "Testing\n", 8);
    } else {
        char buf[100];
        ff_read(fds[1], buf, 8);
        printf("%s", buf);
    }
    x++;
}

int main(int argc, char * argv[])
{
    ff_init(argc, argv);

    int ret = ff_pipe(fds, 0);
    if (ret < 0) {
        printf("ff_socketpair failed, sockfd:%d, errno:%d, %s\n", ret, errno, strerror(errno));
        exit(1);
    }

    ff_run(loop, NULL);
    return 0;
}
