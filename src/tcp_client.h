//
// Created by Molin Liu on 16/5/23.
//

#ifndef PROPERTYTREE_TCP_CLIENT_H
#define PROPERTYTREE_TCP_CLIENT_H
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

class TcpClient {
public:
    TcpClient() = default;

    bool connect_to_server(std::string server_addr, uint64_t port);
    bool send_buffer(char *buffer, size_t size);

private:
    int socket_fd_{};
    struct sockaddr_in serverAddress;
};


#endif //PROPERTYTREE_TCP_CLIENT_H
