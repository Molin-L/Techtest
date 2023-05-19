//
// Created by Molin Liu on 18/5/23.
//
#include <filesystem>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "property_tree.h"
#include "flatbuffers/util.h"
#include "flatbuffers/reflection.h"
#include "flatbuffers/reflection_generated.h"

PropertyTree::PropertyTreeHelper::PropertyTreeHelper(char* buffer, size_t size) {
    this->buffer.resize(size);
    std::copy(buffer, buffer + size, this->buffer.begin());
}

void PropertyTree::PropertyTreeHelper::SetName(std::string name) {
    std::string bfbsfile;
    std::filesystem::path cwd = std::filesystem::current_path();
    std::string bfbsfile_path = cwd.string().append("/../schema/property.bfbs");
    assert(flatbuffers::LoadFile((bfbsfile_path).c_str(),
                                 true, &bfbsfile));
    flatbuffers::Verifier  verifier(reinterpret_cast<const uint8_t*>(bfbsfile.c_str()), bfbsfile.length());
    assert(VerifyPropertyBuffer(verifier));
    auto &schema = *reflection::GetSchema(bfbsfile.c_str());
    auto root_table = schema.root_table();

    auto &name_field = *root_table->fields()->LookupByKey("name");
    auto rroot = flatbuffers::piv(flatbuffers::GetAnyRoot(buffer.data()),
                                  buffer);

    flatbuffers::SetString(schema, name, GetFieldS(**rroot, name_field),
              &buffer);
}

void PropertyTree::PropertyTreeHelper::SetType(Type type) {
    auto property = PropertyTree::GetMutableProperty(this->buffer.data());
    property->mutate_type(type);

    UpdateBuffer(property);
}

void PropertyTree::PropertyTreeHelper::UpdateBuffer(PropertyTree::Property *property) {
    flatbuffers::FlatBufferBuilder fbb;
    auto object_builder = PropertyBuilder(fbb);
    auto property_t = property->UnPack();
    fbb.Finish(Property::Pack(fbb, property_t));

    this->buffer.resize(fbb.GetSize());
    std::copy(fbb.GetBufferPointer(), fbb.GetBufferPointer() + fbb.GetSize(), this->buffer.begin());

    this->callback();
}

void PropertyTree::PropertyTreeHelper::SetCallback(void (*callback)()) {
    this->callback = callback;
}

PropertyTree::Property *PropertyTree::PropertyTreeHelper::GetMutableProperty() {
    return PropertyTree::GetMutableProperty(this->buffer.data());
}

const PropertyTree::Property *PropertyTree::PropertyTreeHelper::GetProperty() {
    return PropertyTree::GetProperty(this->buffer.data());
}

std::vector<char> PropertyTree::PropertyTreeHelper::GetBuffer() {
    return {buffer.begin(), buffer.end()};
}

void PropertyTree::PropertyTreeHelper::Reset(char *in_buffer, int size) {
    this->buffer.resize(size);
    std::copy(in_buffer, in_buffer + size, this->buffer.begin());
}


bool PropertyTree::PropertyTreeMgr::Publish() {
    if (!this->data_) {
        std::cerr << "Must already have property, then can publish it." <<std::endl;
        return false;
    }

    return this->StartSubsServer(8455);
}

bool PropertyTree::PropertyTreeMgr::Subscribe(std::string subs_addr, int port) {
    if (this->connect_to(subs_addr, port)) {
        this->sync_thread_ = std::thread(&::PropertyTree::PropertyTreeMgr::ListenSync, this);
        return true;
    }
    std::cerr<<"Cannot connect to server"<<std::endl;
    return false;
}

bool PropertyTree::PropertyTreeMgr::StartSubsServer(int port) {
    if (subscribe_server_running) {
        std::cout << "Server is already running." << std::endl;
        return false;
    }

    // Create a socket
    self_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (self_fd_ == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return false;
    }

    // Prepare the sockaddr_in structure
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(port);

    // Bind the socket to the specified IP and port
    if (bind(self_fd_, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr)) < 0) {
        std::cerr << "Server Failed to bind socket." << std::endl;
        return false;
    }

    // Start listening for incoming connections
    if (listen(self_fd_, 3) < 0) {
        std::cerr << "Failed to listen for connections." << std::endl;
        return false;
    }

    std::cout << "Server listening on port " << port << std::endl;

    // Start the listening thread
    subscribe_server_running = true;
    listening_thread_ = std::thread(&::PropertyTree::PropertyTreeMgr::ListenConn, this);
    return true;
}

void PropertyTree::PropertyTreeMgr::ListenConn() {
    while (subscribe_server_running) {
        sockaddr_in incoming_addr{};
        socklen_t incoming_addr_len = sizeof(incoming_addr);

        // Accept an incoming connection
        incoming_fd_ = accept(self_fd_, reinterpret_cast<struct sockaddr *>(&incoming_addr), &incoming_addr_len);
        if (incoming_fd_ < 0) {
            std::cerr << "Failed to accept connection." << std::endl;
            continue;
        }

        std::cout << "New client connected" << std::endl;
        this->subs.push_back(incoming_fd_);

        // Send the init data
        auto buffer = this->data_->GetBuffer().data();
        auto size = this->data_->GetBuffer().size();
        this->send_buffer_(incoming_fd_, this->GetData()->GetBuffer().data(), this->GetData()->GetBuffer().size());
    }
}

void PropertyTree::PropertyTreeMgr::stop() {
    if (!subscribe_server_running) {
        if (!is_running_) {
            std::cout << "Not running" << std::endl;
            return;
        }
        is_running_ = false;
        std::cout << "Closing receiver" << std::endl;
        sync_thread_.join();
        return;
    }
    std::cout << "Closing sender" << std::endl;

    subscribe_server_running = false;
    for (auto sub : this->subs) {
        close(sub);
    }
    if (broadcast_thread_.joinable()) {
        broadcast_thread_.join();
    }

}

void PropertyTree::PropertyTreeMgr::Broadcast() {
    for (auto sub : this->subs) {
        this->send_buffer_(sub, this->GetData()->GetBuffer().data(), this->GetData()->GetBuffer().size());
    }
}

void PropertyTree::PropertyTreeMgr::ListenSync() {
    while (is_running_) {
        char buffer[1024];
        ssize_t num_bytes;

        while ((num_bytes = recv(socket_fd_, buffer, sizeof(buffer), 0)) > 0) {
            if (this->data_ == nullptr) {
                this->data_ = std::make_shared<PropertyTreeHelper>(buffer, sizeof(buffer));
            } else {
                this->data_->Reset(buffer, sizeof(buffer));
            }
        }
        if (num_bytes == 0) {
            std::cout << "Incoming connection disconnected" << std::endl;
            return;
        }
    }
}

PropertyTree::PropertyTreeMgr::PropertyTreeMgr(std::shared_ptr<PropertyTreeHelper> helper) {
    this->data_ = helper;
}

void PropertyTree::PropertyTreeMgr::Finish() {
    //this->broadcast_thread_ = std::thread(&PropertyTree::PropertyTreeMgr::Broadcast, this);
    this->Broadcast();
}

PropertyTree::PropertyTreeMgr::~PropertyTreeMgr() {
    stop();
}
