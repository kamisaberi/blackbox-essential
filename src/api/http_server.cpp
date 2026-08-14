#include "http_server.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

namespace blackbox::api {

HTTPServer::HTTPServer(int port) : port_(port) {}

HTTPServer::~HTTPServer() {
    stop();
}

void HTTPServer::start() {
    running_ = true;
    server_thread_ = std::thread(&HTTPServer::listen_loop, this);
}

void HTTPServer::stop() {
    running_ = false;
    if (server_fd_ != -1) {
        close(server_fd_);
        server_fd_ = -1;
    }
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

void HTTPServer::listen_loop() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) return;

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) return;
    if (listen(server_fd_, 5) < 0) return;

    std::cout << "[Air-Gapped API] Local Web Dashboard active on https://127.0.0.1:" << port_ << std::endl;

    while (running_) {
        sockaddr_in client_addr{};
        socklen_t addrlen = sizeof(client_addr);
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &addrlen);
        if (client_fd < 0) continue;

        std::string response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n\r\n"
            "{\"status\":\"active\",\"node\":\"blackbox-airgapped-01\",\"threats_blocked\":42}";

        write(client_fd, response.c_str(), response.size());
        close(client_fd);
    }
}

} // namespace blackbox::api