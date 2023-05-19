#ifndef TCP_SERVICE_H
#define TCP_SERVICE_H

#include <memory>
#include <vector>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <netinet/in.h>
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/reflection.h"

class TcpConn {
public:
    TcpConn() = default;

    virtual ~TcpConn();
    explicit TcpConn(uint64_t port);
    void start_server();
    void start_server_with_handler(void (*func)(char*, int));

    virtual void stop();
    bool connect_to(std::string server_addr, uint64_t port);
    bool send_buffer(char *buffer, size_t size);
    bool send_buffer_(int conn_fd, char *buffer, size_t size);
    void listen_for_incoming_data();
    void handle_incoming_data(int incoming_fd);
    virtual void before_handle(int incoming_fd) {};
    void (*handler)(char*, int) = nullptr;

protected:
    uint64_t port_{};
    int socket_fd_{};
    struct sockaddr_in serverAddress;
    int self_fd_{};
    int client_fd_{};
    int incoming_fd_{};
    bool is_running_{};
    std::thread listening_thread_;
};

#endif // TCP_SERVICE_H