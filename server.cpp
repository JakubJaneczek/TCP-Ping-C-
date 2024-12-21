#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

void runTCPServer(int port) {
    int sockfd, newsockfd;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "ERROR opening socket\n";
        exit(1);
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "ERROR on binding\n";
        close(sockfd);
        exit(1);
    }

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
    if (newsockfd < 0) {
        std::cerr << "ERROR on accept\n";
        close(sockfd);
        exit(1);
    }

    for (int i = 0; i < 1000; ++i) { 
        bzero(buffer, 256);
        n = read(newsockfd, buffer, 255);
        if (n < 0) {
            std::cerr << "ERROR reading from socket\n";
        }
        std::cout << "Received ping: " << buffer << "\n";

        // Send response
        n = write(newsockfd, "Pong", 4);
        if (n < 0) {
            std::cerr << "ERROR writing to socket\n";
        }
    }

    close(newsockfd);
    close(sockfd);
}

void runUDPServer(int port) {
    int sockfd;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    int n;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "ERROR opening socket\n";
        exit(1);
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "ERROR on binding\n";
        close(sockfd);
        exit(1);
    }

    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        std::cerr << "ERROR setting timeout\n";
        close(sockfd);
        exit(1);
    }

    clilen = sizeof(cli_addr);

    for (int i = 0; i < 1000; ++i) {
        bzero(buffer, 256);

        n = recvfrom(sockfd, buffer, 255, 0, (struct sockaddr*)&cli_addr, &clilen);
        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                std::cout << "Timeout reached, no packet received\n";
                continue;  // Skip to the next iteration
            }
            else {
                std::cerr << "ERROR in recvfrom\n";
                break;
            }
        }
        std::cout << "Received ping: " << buffer << "\n";

        n = sendto(sockfd, "Pong", 4, 0, (struct sockaddr*)&cli_addr, clilen);
        if (n < 0) {
            std::cerr << "ERROR in sendto\n";
        }
    }

    close(sockfd);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <tcp|udp> <port>\n";
        exit(1);
    }

    std::string protocol = argv[1];
    int port = std::stoi(argv[2]);

    if (protocol == "tcp") {
        runTCPServer(port);
    }
    else if (protocol == "udp") {
        runUDPServer(port);
    }
    else {
        std::cerr << "Invalid protocol. Use 'tcp' or 'udp'.\n";
        exit(1);
    }

    return 0;
}
