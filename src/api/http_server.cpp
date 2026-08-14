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
    server_thread_ = std::thread([this]() {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) return;

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            close(server_fd);
            return;
        }

        listen(server_fd, 5);
        std::cout << "[HTTP Server] Air-Gapped Web Dashboard active on port: " << port_ << std::endl;

        while (running_) {
            int new_socket = accept(server_fd, nullptr, nullptr);
            if (new_socket >= 0) {
                const char* response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"blackbox_active\"}";
                send(new_socket, response, strlen(response), 0);
                close(new_socket);
            }
        }
        close(server_fd);
    });
}

void HTTPServer::stop() {
    running_ = false;
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

} // namespace blackbox::api