#pragma once
#include <thread>
#include <atomic>

namespace blackbox::api {

class HTTPServer {
public:
    explicit HTTPServer(int port);
    ~HTTPServer();

    void start();
    void stop();

private:
    int port_;
    std::atomic<bool> running_{false};
    std::thread server_thread_;
};

} // namespace blackbox::api