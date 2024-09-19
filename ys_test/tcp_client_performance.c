#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<sys/time.h>
#include <netinet/tcp.h>
#include <signal.h>



#define SERVER_IP "192.168.182.5" // F-SATCK IP地址
// #define SERVER_IP "192.168.182.135" // 本机IP地址
#define SERVER_PORT 8080 // f-stack 端口
#define BUFFER_SIZE 1000 * 10
char buffer[BUFFER_SIZE];
long int sum = 0;

void handle_alarm(int sig)
{
    (void)sig;
    printf("throughput:%lfGB/s\n", sum/1000/1000/1000.0/5);
    sum= 0;
    alarm(5);
}

int main() {
    int sock;
    struct sockaddr_in server_addr, local_addr;

    // 创建套接字
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr("192.168.182.4"); // 使用默认网络接口
    local_addr.sin_port = htons(46000);
    bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr));

    // 设置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    // 将IP地址转换为二进制形式
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    // 连接到服务器
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    int sndbuf_size = 1024 * 256, opt = 1, max_meg = 3276;
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sndbuf_size, sizeof(sndbuf_size));
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    int ret = 0;
    ret = setsockopt(sock, IPPROTO_TCP, TCP_MAXSEG, &max_meg, sizeof(max_meg));
    if(ret < 0)
    {
        printf("change failed\n");
    }


    socklen_t optlen;
    int rcvbuf_actual, sndbuf_actual, mss;
    optlen = sizeof(rcvbuf_actual);
    getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf_actual, &optlen);
    getsockopt(sock, SOL_SOCKET,SO_SNDBUF, &sndbuf_actual, &optlen);
    getsockopt(sock, IPPROTO_TCP, TCP_MAXSEG, &mss, &optlen);
    printf("RCVBUF:%d\nSNDBUF:%d\nMSS:%d\n", rcvbuf_actual, sndbuf_actual,mss);

    // 初始化发送缓冲区
    memset(buffer, 'A', BUFFER_SIZE - 1);
    buffer[BUFFER_SIZE-1] = '\0'; 

    struct timeval time_ahead, time_after;
    gettimeofday(&time_ahead,NULL);
    int sendlen = 0;
    long int  i = 0, pkt_num = 1000* 1000 * 1000;
    
    signal(SIGALRM, handle_alarm);
    alarm(5);
    while (i < pkt_num) {
        // if(i % 10000 == 0)
        // {
        //     printf("%d\n",i);
        // }
        // 发送数据
        if ((sendlen = write(sock, buffer, BUFFER_SIZE)) > 0) {
            sum += sendlen;
            // printf("sendlen:%d\n",sendlen);
        }
        else
        {
            perror("Send failed");
            exit(EXIT_FAILURE);
        }

        // printf("send bytes:%d\n",n);
        // 为了避免占用过多CPU资源，可以加入适当的延时
        // usleep(1); // 1毫秒延时
        i ++;
    }
    // printf("sum:%ld\n",sum);
    // double result = sum / 1000000.0;
    // gettimeofday(&time_after, NULL);



    // // double delta = (time_after.tv_usec - time_ahead.tv_usec)/1000000.0;
    // double delta = time_after.tv_sec - time_ahead.tv_sec + (time_after.tv_usec - time_ahead.tv_usec)/ 1000000.0;
    // printf("Throughput:%.6lfMb/s\n", result/delta);
    // 关闭套接字
    close(sock);

    return 0;
}
