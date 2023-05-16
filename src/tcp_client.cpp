//
// Created by Molin Liu on 16/5/23.
//

#include <iostream>
#include <arpa/inet.h>
#include "tcp_client.h"


bool TcpClient::connect_to_server(std::string server_addr, uint64_t port) {
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
    return true;
}

bool TcpClient::send_buffer(char *buffer, size_t size) {
    if (send(socket_fd_, buffer, size, 0) < 0) {
        std::cerr << "Failed to send data." << std::endl;
        return false;
    }

    return true;
}

