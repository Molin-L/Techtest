#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tcp_conn.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/reflection.h"
#include "schema/property_generated.h"

void TcpConn::start_server() {
    if (is_running_) {
        std::cout << "Server is already running." << std::endl;
        return;
    }

    // Create a socket
    self_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (self_fd_ == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return;
    }

    // Prepare the sockaddr_in structure
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(port_);

    // Bind the socket to the specified IP and port
    if (bind(self_fd_, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket." << std::endl;
        return;
    }

    // Start listening for incoming connections
    if (listen(self_fd_, 3) < 0) {
        std::cerr << "Failed to listen for connections." << std::endl;
        return;
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    // Start the listening thread
    is_running_ = true;
    listening_thread_ = std::thread(&::TcpConn::listen_for_incoming_data, this);
}

void TcpConn::stop() {
    if (!is_running_) {
        std::cout << "Server is not running." << std::endl;
        return;
    }

    std::cout << "Stopping server..." << std::endl;
    // Stop the listening thread
    is_running_ = false;
    if (listening_thread_.joinable()) {
        listening_thread_.join();
    }

    if (self_fd_ != 0) {
        close(self_fd_);
        self_fd_ = 0;
    }
    std::cout << "Server stopped." << std::endl;
}

void TcpConn::listen_for_incoming_data() {
    while (is_running_) {
        sockaddr_in incoming_addr{};
        socklen_t incoming_addr_len = sizeof(incoming_addr);

        // Accept an incoming connection
        incoming_fd_ = accept(self_fd_, reinterpret_cast<struct sockaddr *>(&incoming_addr), &incoming_addr_len);
        if (incoming_fd_ < 0) {
            std::cerr << "Failed to accept connection." << std::endl;
            continue;
        }

        std::cout << "New client connected" << std::endl;

        this->before_handle(incoming_fd_);

        // Handle the client connection
        this->handle_incoming_data(incoming_fd_);

        // Close the client socket
        close(incoming_fd_);
        incoming_fd_ = 0;
    }
}

void TcpConn::handle_incoming_data(int incoming_fd) {
    char buffer[1024];
    int num_bytes;

    while ((num_bytes = recv(incoming_fd, buffer, sizeof(buffer), 0)) > 0) {
        this->handler(buffer, num_bytes);
        break;
    }

    if (num_bytes == 0) {
        std::cout << "Incoming connection disconnected" << std::endl;
    }
}

TcpConn::~TcpConn() {
    stop();
}

void TcpConn::start_server_with_handler(void (*func)(char *, int)) {
    this->handler = func;
    return this->start_server();
}

TcpConn::TcpConn(uint64_t port) : port_(port), is_running_(false), self_fd_(0), client_fd_(0) {
    this->handler = [](char *buffer, int len) {
        auto property = PropertyTree::GetProperty(buffer);
        if (property) {
            std::cout << "Received property: " << property->name()->str() << std::endl;
        }
    };

}

bool TcpConn::send_buffer(char *buffer, size_t size) {
    return this->send_buffer_(socket_fd_, buffer, size);
}

bool TcpConn::connect_to(std::string server_addr, uint64_t port) {
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return false;
    }
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(server_addr.c_str());
    serverAddress.sin_port = htons(port);

    if (connect(socket_fd_, reinterpret_cast<struct sockaddr *>(&serverAddress), sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to connect to server." << std::endl;
        return false;
    }

    is_running_ = true;
    return true;
}

bool TcpConn::send_buffer_(int conn_fd, char *buffer, size_t size) {
    if (send(conn_fd, buffer, size, 0) < 0) {
        std::cerr << "Failed to send data." << std::endl;
        return false;
    }

    return true;
}
