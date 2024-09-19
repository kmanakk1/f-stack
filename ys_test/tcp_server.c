#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netinet/tcp.h>

#define PORT 8080
#define BUFFER_SIZE 1024*100
long int sum = 0;
char buffer[BUFFER_SIZE] = {0};

void handle_alarm(int sig)
{
    (void)sig;
    printf("throughput:%lfGB/s\n", sum/1000/1000/1000.0/5);
    sum= 0;
    alarm(5);
}


int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    const char *hello = "Hello from server";

    // 创建 socket 文件描述符
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    

    // 绑定到指定的 IP 地址和端口
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("192.168.182.3");
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 监听连接
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    // 接受连接
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("accept failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
        int sndbuf_size = 1024 * 256, opt = 1, max_meg = 3276;
    setsockopt(new_socket, SOL_SOCKET, SO_SNDBUF, &sndbuf_size, sizeof(sndbuf_size));
    setsockopt(new_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    int ret = 0;
    ret = setsockopt(new_socket, IPPROTO_TCP, TCP_MAXSEG, &max_meg, sizeof(max_meg));
    if(ret < 0)
    {
        printf("change failed\n");
    }


    socklen_t optlen;
    int rcvbuf_actual, sndbuf_actual, mss;
    optlen = sizeof(rcvbuf_actual);
    getsockopt(new_socket, SOL_SOCKET, SO_RCVBUF, &rcvbuf_actual, &optlen);
    getsockopt(new_socket, SOL_SOCKET,SO_SNDBUF, &sndbuf_actual, &optlen);
    getsockopt(new_socket, IPPROTO_TCP, TCP_MAXSEG, &mss, &optlen);
    printf("RCVBUF:%d\nSNDBUF:%d\nMSS:%d\n", rcvbuf_actual, sndbuf_actual,mss);

    signal(SIGALRM, handle_alarm);
    alarm(5);
    while(1)
    {
        int valread = read(new_socket, buffer, BUFFER_SIZE);
        sum += valread;
    }

    // 关闭套接字
    close(new_socket);
    close(server_fd);

    return 0;
}
