#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tcp_server.h"
#include "schema/property_generated.h"

void TcpServer::start_server() {
    if (is_running_) {
        std::cout << "Server is already running." << std::endl;
        return;
    }

    // Create a socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return;
    }

    // Prepare the sockaddr_in structure
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(port_);

    // Bind the socket to the specified IP and port
    if (bind(server_fd_, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket." << std::endl;
        return;
    }

    // Start listening for incoming connections
    if (listen(server_fd_, 3) < 0) {
        std::cerr << "Failed to listen for connections." << std::endl;
        return;
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    // Start the listening thread
    is_running_ = true;
    listening_thread_ = std::thread(&::TcpServer::listen_for_client, this);
}

void TcpServer::stop() {
    if (!is_running_) {
        std::cout << "Server is not running." << std::endl;
        return;
    }

    std::cout << "Stopping server..." << std::endl;
    // Stop the listening thread
    is_running_ = false;
    listening_thread_.join();

    if (server_fd_ != 0) {
        close(server_fd_);
        server_fd_ = 0;
    }
    std::cout << "Server stopped." << std::endl;
}

void TcpServer::listen_for_client() {
    while (is_running_) {
        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);

        // Accept a client connection
        client_fd_ = accept(server_fd_, reinterpret_cast<struct sockaddr *>(&client_addr), &client_addr_len);
        if (client_fd_ < 0) {
            std::cerr << "Failed to accept connection." << std::endl;
            continue;
        }

        std::cout << "New client connected" << std::endl;

        // Handle the client connection
        this->handle_client();

        // Close the client socket
        close(client_fd_);
        client_fd_ = 0;
    }
}

void TcpServer::handle_client() {
    char buffer[1024];
    int num_bytes;

    while ((num_bytes = recv(client_fd_, buffer, sizeof(buffer), 0)) > 0) {
        auto property = PropertyTree::GetProperty(buffer);
        if (property) {
            std::cout << "Received property: " << property->name()->str() << std::endl;
            break;
        }
    }

    if (num_bytes == 0) {
        std::cout << "Client disconnected" << std::endl;
    }
}

TcpServer::~TcpServer() {
    stop();
}
