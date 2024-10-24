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

using namespace std::chrono;

void printCurrentTimestamp() {
    auto now = system_clock::now();
    std::time_t currentTime = system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&currentTime);
    std::cout << "[" << std::put_time(localTime, "%Y-%m-%d %H:%M:%S") << "] ";
}

void runTCPClient(const std::string& address, int port) {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[256];
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
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

    std::cout << "Average RTT: " << totalDuration / 100 << " microseconds\n";
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
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <tcp|udp> <server_ip> <port>\n";
        exit(1);
    }

    std::string protocol = argv[1];
    std::string server_ip = argv[2];
    int port = std::stoi(argv[3]);

    if (protocol == "tcp") {
        runTCPClient(server_ip, port);
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
