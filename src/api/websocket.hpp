#pragma once
#include <thread>
#include <atomic>

namespace blackbox::api {

class WebSocketServer {
public:
    explicit WebSocketServer(int port);
    ~WebSocketServer();

    void start();
    void stop();

private:
    int port_;
    std::atomic<bool> running_{false};
    std::thread ws_thread_;
};

} // namespace blackbox::api