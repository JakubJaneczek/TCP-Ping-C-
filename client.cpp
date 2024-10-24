#include <iostream>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>
#include <iomanip>
#include <netinet/tcp.h>

using namespace std::chrono;

// Function to print the current timestamp
void printCurrentTimestamp() {
    auto now = system_clock::now();
    std::time_t currentTime = system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&currentTime);
    std::cout << "[" << std::put_time(localTime, "%Y-%m-%d %H:%M:%S") << "] ";
}

// Function to modify the TCP socket (optional)
void modifyTCPSocket(int sockfd, bool enableNoDelay, int sendBufferSize, int recvBufferSize) {
    // Example 1: Disable Nagle's algorithm (TCP_NODELAY)
    if (enableNoDelay) {
        int flag = 1;
        if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (void*)&flag, sizeof(int)) < 0) {
            std::cerr << "ERROR: Couldn't set TCP_NODELAY\n";
        }
        else {
            std::cout << "TCP_NODELAY enabled.\n";
        }
    }

    // Example 2: Set socket send buffer size (SO_SNDBUF)
    if (sendBufferSize > 0) {
        if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sendBufferSize, sizeof(sendBufferSize)) < 0) {
            std::cerr << "ERROR: Couldn't set SO_SNDBUF\n";
        }
        else {
            std::cout << "Send buffer size set to " << sendBufferSize << " bytes.\n";
        }
    }

    // Example 3: Set socket receive buffer size (SO_RCVBUF)
    if (recvBufferSize > 0) {
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvBufferSize, sizeof(recvBufferSize)) < 0) {
            std::cerr << "ERROR: Couldn't set SO_RCVBUF\n";
        }
        else {
            std::cout << "Receive buffer size set to " << recvBufferSize << " bytes.\n";
        }
    }
}

void runTCPClient(const std::string& address, int port, bool modifySocket = false, bool enableNoDelay = false, int sendBufferSize = 0, int recvBufferSize = 0) {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "ERROR opening socket\n";
        exit(1);
    }

    // Modify the socket options if requested
    if (modifySocket) {
        modifyTCPSocket(sockfd, enableNoDelay, sendBufferSize, recvBufferSize);
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, address.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported\n";
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed\n";
        close(sockfd);
        exit(1);
    }

    long totalDuration = 0;
    for (int i = 0; i < 1000; ++i) {
        auto start = high_resolution_clock::now();

        std::string message = "Ping " + std::to_string(i + 1);
        int n = write(sockfd, message.c_str(), message.length());
        if (n < 0) {
            std::cerr << "ERROR writing to socket\n";
        }

        bzero(buffer, 256);
        n = read(sockfd, buffer, 255);
        if (n < 0) {
            std::cerr << "ERROR reading from socket\n";
        }

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);
        totalDuration += duration.count();

        printCurrentTimestamp();
        std::cout << "Ping " << i + 1 << " RTT: " << duration.count() << " microseconds\n";
    }

    std::cout << "Average RTT: " << totalDuration / 1000 << " microseconds\n";
    close(sockfd);
}

void runUDPClient(const std::string& address, int port) {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[256];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "ERROR opening socket\n";
        exit(1);
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, address.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported\n";
        exit(1);
    }

    socklen_t serv_len = sizeof(serv_addr);

    // Set receive timeout for the socket
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        std::cerr << "ERROR setting timeout\n";
        close(sockfd);
        exit(1);
    }

    long totalDuration = 0;
    for (int i = 0; i < 1000; ++i) {
        auto start = high_resolution_clock::now();

        std::string message = "Ping " + std::to_string(i + 1);
        int n = sendto(sockfd, message.c_str(), message.length(), 0, (struct sockaddr*)&serv_addr, serv_len);
        if (n < 0) {
            std::cerr << "ERROR in sendto\n";
        }

        bzero(buffer, 256);
        n = recvfrom(sockfd, buffer, 255, 0, (struct sockaddr*)&serv_addr, &serv_len);
        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                printCurrentTimestamp();
                std::cout << "Timeout reached, no response for Ping " << i + 1 << "\n";
                continue;
            }
            else {
                std::cerr << "ERROR in recvfrom\n";
                break;
            }
        }

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);
        totalDuration += duration.count();

        printCurrentTimestamp();
        std::cout << "Ping " << i + 1 << " RTT: " << duration.count() << " microseconds\n";
    }

    std::cout << "Average RTT: " << totalDuration / 1000 << " microseconds\n";
    close(sockfd);
}

int main(int argc, char* argv[]) {
    if (argc != 4 && argc != 7) {
        std::cerr << "Usage: " << argv[0] << " <tcp|udp> <server_ip> <port> [enableNoDelay] [sendBufferSize] [recvBufferSize]\n";
        exit(1);
    }

    std::string protocol = argv[1];
    std::string server_ip = argv[2];
    int port = std::stoi(argv[3]);

    // Optional socket modification settings for TCP
    bool enableNoDelay = argc >= 5 ? std::stoi(argv[4]) : false;
    int sendBufferSize = argc >= 6 ? std::stoi(argv[5]) : 0;
    int recvBufferSize = argc >= 7 ? std::stoi(argv[6]) : 0;

    if (protocol == "tcp") {
        bool modifySocket = (argc == 7);  // Enable modification if more arguments are passed
        runTCPClient(server_ip, port, modifySocket, enableNoDelay, sendBufferSize, recvBufferSize);
    }
    else if (protocol == "udp") {
        runUDPClient(server_ip, port);
    }
    else {
        std::cerr << "Invalid protocol. Use 'tcp' or 'udp'.\n";
        exit(1);
    }

    return 0;
}
