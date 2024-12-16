#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 8080  // 服务器监听的端口
#define BUFFER_SIZE 1024 * 100  // 接收数据的缓冲区大小

double speed_cal(struct timeval *time_start, struct timeval *time_end, uint64_t byte_cnt)
{
    double delta_time = (time_end->tv_sec - time_start->tv_sec) + (time_end->tv_usec - time_start->tv_usec)/(1000000.0);
    double speed = (byte_cnt/(1024 * 1024 * 1024.0)) / delta_time;
    return speed;
}


int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in servaddr, cliaddr;

    // 创建套接字
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 清空并设置服务器地址
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;  // 使用 IPv4
    servaddr.sin_addr.s_addr = inet_addr("192.168.182.4");  // 监听所有网络接口
    servaddr.sin_port = htons(PORT);  // 设置端口

    // 绑定套接字到指定端口
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int n;
    socklen_t len = sizeof(cliaddr);  // 地址结构体大小

    printf("UDP server is listening on port %d...\n", PORT);
    uint64_t byte_cnt = 0, threshold = 1024 * 1024 * 1024;
    struct timeval time_start, time_end;
    double speed;

    gettimeofday(&time_start, NULL);
    while (1) {
        // 接收数据
        n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, 
                     (struct sockaddr *)&cliaddr, &len);
        byte_cnt += n;
        if(byte_cnt == threshold * 2)
        {
            gettimeofday(&time_end, NULL);
            speed = speed_cal(&time_start, &time_end, byte_cnt);
            printf("speed: %lfGB/s\n", speed);
            
            byte_cnt = 0;
            gettimeofday(&time_start, NULL);
        }
        // buffer[n] = '\0';  // 添加字符串结束符
        // printf("Client: %s\n", buffer);

        // // 发送响应
        // const char *response = "Hello from server";
        // sendto(sockfd, response, strlen(response), MSG_CONFIRM, 
        //        (const struct sockaddr *)&cliaddr, len);
        // printf("Response sent to client.\n");
    }

    // 关闭套接字
    close(sockfd);
    return 0;
}
