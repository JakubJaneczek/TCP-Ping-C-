#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// Constants to replace magic numbers
constexpr int BUFFER_SIZE = 256;
constexpr int NUM_PINGS = 1000;
constexpr int SOCKET_BACKLOG = 5;
constexpr int TIMEOUT_SEC = 2;
constexpr int TIMEOUT_USEC = 0;

int acceptTCPConnection(int port) {
    int sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "ERROR opening socket\n";
        exit(1);
    }

    // Set up the server address structure
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    // Bind the socket to the port
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "ERROR on binding\n";
        close(sockfd);
        exit(1);
    }

    // Listen for incoming connections
    listen(sockfd, SOCKET_BACKLOG);
    clilen = sizeof(cli_addr);

    // Accept a client connection
    newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
    if (newsockfd < 0) {
        std::cerr << "ERROR on accept\n";
        close(sockfd);
        exit(1);
    }

    close(sockfd); // Close the listening socket
    return newsockfd;
}

void runTCPServer(int port, int hs, int pings) {
    for (int j = 0; j < hs; ++j) {
        int newsockfd;
        char buffer[BUFFER_SIZE];
        int n;

        // Perform the TCP handshake and get the connected socket
        newsockfd = acceptTCPConnection(port+j);

        // Communication loop
        for (int i = 0; i < pings; ++i) {
            bzero(buffer, BUFFER_SIZE);
            n = read(newsockfd, buffer, BUFFER_SIZE - 1);
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
    }
}


void runUDPServer(int port) {
    int sockfd;
    char buffer[BUFFER_SIZE];
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

    // Set receive timeout
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = TIMEOUT_USEC;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        std::cerr << "ERROR setting timeout\n";
        close(sockfd);
        exit(1);
    }

    clilen = sizeof(cli_addr);

    for (int i = 0; i < NUM_PINGS; ++i) {
        bzero(buffer, BUFFER_SIZE);

        n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&cli_addr, &clilen);
        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                std::cout << "Timeout reached, no packet received\n";
                continue;
            } else {
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
        std::cerr << "Usage: " << argv[0] << " <tcp|udp|tcp_hs> <port>\n";
        exit(1);
    }

    std::string protocol = argv[1];
    int port = std::stoi(argv[2]);

    if (protocol == "tcp") {
        runTCPServer(port, 1, NUM_PINGS);
    } else if(protocol == "tcp_hs"){
    	runTCPServer(port, 100, 1);
    } else if (protocol == "udp") {
        runUDPServer(port);
    } else {
        std::cerr << "Invalid protocol. Use 'tcp' or 'udp'.\n";
        exit(1);
    }

    return 0;
}

