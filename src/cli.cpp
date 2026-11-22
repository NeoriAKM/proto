#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket failed");
        return 1;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        return 1;
    }
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        return 1;
    }

    char buffer[1024] = {0};
    read(sock, buffer, sizeof(buffer) - 1);
    std::cout << buffer;

    std::string name;
    std::getline(std::cin, name);
    write(sock, (name + '\n').c_str(), name.length()+1);

    // SUCCESS //

    std::cout << "Connection success!\n";

    std::thread reader([sock]() {
        char buf[1024];
        while (true) {
            int n = read(sock, buf, sizeof(buf) - 1);
            if (n <= 0) break;
            buf[n] = '\0';
            std::cout << buf;
        }
    });
    reader.detach();

    std::string msg;
    while (std::getline(std::cin, msg)) {
        if (msg == "quit") break;
        write(sock, (msg + "\n").c_str(), msg.length() + 1);
    }

    close(sock);
    return 0;
}