#pragma once
#include <string>
#include <atomic>
#include <thread>

namespace blackbox::api {

class HTTPServer {
public:
    explicit HTTPServer(int port);
    ~HTTPServer();

    void start();
    void stop();

private:
    void listen_loop();

    int port_;
    int server_fd_{-1};
    std::atomic<bool> running_{false};
    std::thread server_thread_;
};

} // namespace blackbox::api