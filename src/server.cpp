#include <iostream>
#include <netinet/in.h>
#include <thread>
#include <csignal>
#include <vector>
#include <mutex>
#include <algorithm>
#include <unistd.h>
#include <sys/socket.h>
#include <string>

struct Client {
    int fd;
    std::string name;
};

std::vector<Client> clients;
std::mutex clients_mutex;

void broadcast_message(const std::string& message, int sender_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto& client : clients) {
        if (client.fd != sender_fd) {
            write(client.fd, message.c_str(), message.length());
        }
    }
}

void handle_client(int client_fd) {
    char buffer[1024];

    write(client_fd, "Enter your name: ", 17);
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }

    buffer[bytes_read - 1] = '\0';
    std::string client_name(buffer);
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.push_back({client_fd, client_name});
    }

    std::cout << "SERVER: " << client_name << " connected!\n";
    broadcast_message("[" + client_name + " joined the chat!]\n", client_fd);

    while ((bytes_read = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read - 1] = '\0';

        std::string msg = "[" + client_name + "]: " + buffer + "\n";
        std::cout << msg;

        broadcast_message(msg, client_fd);
    }

    std::cout << "SERVER: " << client_name << " disconnected\n";
    broadcast_message("[" + client_name + " left the chat]\n", client_fd);

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(
            std::remove_if(clients.begin(), clients.end(), [client_fd](const Client& c) { return c.fd == client_fd; }),
            clients.end()
        );
    }
    close(client_fd);
}


int main() {

    signal(SIGPIPE, SIG_IGN);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        return 1;
    }
    std::cout << "Server launched succefully 8080...\n";

    // RUN //

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) continue;
        std::cout << "SERVER: new client\n";
        std::thread(handle_client, client_fd).detach();
    }
    close(server_fd);
    return 0;
}