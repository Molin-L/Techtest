#ifndef TCP_SERVICE_H
#define TCP_SERVICE_H

#include <memory>
#include <vector>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/reflection.h"

class TcpServer {
public:
    TcpServer() = default;

    ~TcpServer();
    explicit TcpServer(uint64_t port) : port_(port), server_fd_(0), client_fd_(0), is_running_(false) {}
    void start_server();
    void stop();

protected:
    void listen_for_client();
    void handle_client();

protected:
    uint64_t port_{};
    int server_fd_{};
    int client_fd_{};
    bool is_running_{};
    std::thread listening_thread_;
};

#endif // TCP_SERVICE_H