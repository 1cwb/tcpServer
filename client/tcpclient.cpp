#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

int main() {
    // 创建客户端socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // 配置服务器地址
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    if(inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sockfd);
        return 1;
    }

    // 建立连接
    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        return 1;
    }

    std::cout << "Connected to server" << std::endl;

    // 通信循环
    char buffer[BUFFER_SIZE];
    while(true) {
        std::cout << "Enter message (q to quit): ";
        std::string input;
        std::getline(std::cin, input);
        
        if(input == "q") break;

        // 发送数据
        ssize_t sent = send(sockfd, input.c_str(), input.size(), 0);
        if(sent < 0) {
            perror("Send failed");
            break;
        }

        // 接收响应
        ssize_t received = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if(received <= 0) {
            perror("Receive failed");
            break;
        }

        std::cout << "Server response: " 
                  << std::string(buffer, received) 
                  << std::endl;
    }

    close(sockfd);
    return 0;
}