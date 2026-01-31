#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8020);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address\n";
        close(sockfd);
        return 1;
    }

    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    std::cout << "Connected to server at 127.0.0.1:8020\n";
    std::string input;

    while (true) {
        std::cout << "Enter message (or 'quit' to exit): ";
        std::getline(std::cin, input);
        if (input == "quit") {
            std::cout << "Exiting.\n";
            break;
        }

        ssize_t sent = send(sockfd, input.c_str(), input.size(), 0);
        if (sent < 0) {
            perror("send");
            break;
        }
        std::cout << "Send success";

        // char buffer[1024] = {0};
        // ssize_t received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        // if (received < 0) {
        //     perror("recv");
        //     break;
        // } else if (received == 0) {
        //     std::cout << "Server closed connection\n";
        //     break;
        // }

        // buffer[received] = '\0';
        // std::cout << "Received from server: " << buffer << "\n";
    }

    close(sockfd);
    return 0;
}
