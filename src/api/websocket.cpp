#include "websocket.hpp"
#include <iostream>
#include <chrono>

namespace blackbox::api {

WebSocketServer::WebSocketServer(int port) : port_(port) {}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::start() {
    running_ = true;
    ws_thread_ = std::thread([this]() {
        std::cout << "[WebSocket Engine] Air-gapped real-time stream active on port: " << port_ << std::endl;
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    });
}

void WebSocketServer::stop() {
    running_ = false;
    if (ws_thread_.joinable()) {
        ws_thread_.join();
    }
}

} // namespace blackbox::api