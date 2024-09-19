#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080  // 服务器监听的端口
#define BUFFER_SIZE 1024  // 接收数据的缓冲区大小

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
    servaddr.sin_addr.s_addr = INADDR_ANY;  // 监听所有网络接口
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

    // 无限循环处理接收和发送
    while (1) {
        // 接收数据
        n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, 
                     (struct sockaddr *)&cliaddr, &len);
        buffer[n] = '\0';  // 添加字符串结束符
        printf("Client: %s\n", buffer);

        // 发送响应
        const char *response = "Hello from server";
        sendto(sockfd, response, strlen(response), MSG_CONFIRM, 
               (const struct sockaddr *)&cliaddr, len);
        printf("Response sent to client.\n");
    }

    // 关闭套接字
    close(sockfd);
    return 0;
}
