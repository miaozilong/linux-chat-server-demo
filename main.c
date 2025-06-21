#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

#define PORT 8080           // 1. 定义服务器监听端口号
#define BUFFER_SIZE 1024    // 2. 定义缓冲区大小

// 3. 处理客户端连接的线程函数
void* handle_client(void* arg) {
    int client_socket = *(int*)arg;  // 4. 获取客户端 socket 描述符
    free(arg);  // 5. 释放传入的堆内存指针

    char buffer[BUFFER_SIZE];  // 6. 定义用于接收消息的缓冲区

    printf("新客户端连接，线程ID: %ld\n", pthread_self());
    fflush(stdout);  // 7. 刷新标准输出，确保打印及时显示

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);  // 8. 清空缓冲区
        ssize_t bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);  // 9. 从客户端读取数据

        if (bytes_read <= 0) {  // 10. 客户端断开连接或出错
            printf("客户端断开，线程ID: %ld\n", pthread_self());
            break;  // 11. 退出循环，关闭连接
        }

        printf("[线程 %ld] 接收到消息: %s\n", pthread_self(), buffer);  // 12. 打印接收到的消息
        fflush(stdout);
    }

    close(client_socket);  // 13. 关闭客户端 socket
    return NULL;  // 14. 线程结束
}

int main() {
    int server_fd;                // 15. 服务器 socket 文件描述符
    struct sockaddr_in address;   // 16. 服务器地址结构体
    int addrlen = sizeof(address);  // 17. 地址长度

    // 18. 创建 socket，使用 IPv4 和 TCP 协议
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 19. 设置 socket 选项，允许地址复用，避免绑定失败
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 20. 配置服务器地址信息
    address.sin_family = AF_INET;          // IPv4
    address.sin_addr.s_addr = INADDR_ANY;  // 监听所有网卡地址
    address.sin_port = htons(PORT);        // 端口号转换为网络字节序

    // 21. 绑定 socket 到指定地址和端口
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 22. 开始监听，设置最大连接队列长度为10
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("服务器已启动，等待客户端连接...\n");
    fflush(stdout);

    while (1) {
        // 23. 为每个新连接分配内存，保存客户端 socket
        int* new_socket = malloc(sizeof(int));
        // 24. 接受客户端连接，阻塞等待
        *new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

        if (*new_socket < 0) {
            perror("accept");
            free(new_socket);
            continue;  // 25. 出错则跳过本次循环，继续等待连接
        }

        // 26. 创建线程处理新的客户端连接
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, new_socket) != 0) {
            perror("pthread_create failed");
            close(*new_socket);
            free(new_socket);
        } else {
            pthread_detach(tid);  // 27. 设置线程分离，自动回收资源
        }
    }

    // 28. 关闭服务器 socket （一般不会执行到这里）
    close(server_fd);
    return 0;
}
