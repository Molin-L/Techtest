//
// Created by Molin Liu on 18/5/23.
//

#ifndef PROPERTYTREE_PROPERTY_TREE_H
#define PROPERTYTREE_PROPERTY_TREE_H

#include <iostream>
#include "schema/property_generated.h"
#include "src/tcp_service/tcp_conn.h"


namespace PropertyTree {
class PropertyTreeMgr;

class PropertyTreeHelper {
public:
    PropertyTreeHelper() = default;

    PropertyTreeHelper(char *buffer, size_t size);

public:
    void SetName(std::string name);
    void SetType(Type type);

    template <typename T>
    void SetValue(T value);

    void AddSubProperty(PropertyTreeHelper *subProperty);

    uint8_t* Serialize();
    uint8_t* Serialize(const char *buffer, int size);

    void SetCallback(void (*callback)());
    void Reset(char* buffer, int size);
    bool Subscribe(std::string server_addr, int port);

    Property *GetMutableProperty();
    const Property *GetProperty();
    std::vector<char> GetBuffer();

    void SetMgr(PropertyTree::PropertyTreeMgr *mgr);

private:
    void UpdateBuffer(PropertyTree::Property *property);

    flatbuffers::FlatBufferBuilder builder;
    std::vector<uint8_t> buffer;
    void (*callback)() = [](){};
};

class PropertyTreeMgr : public TcpConn {
public:
    PropertyTreeMgr() = default;
    PropertyTreeMgr(std::shared_ptr<PropertyTreeHelper> helper);

    ~PropertyTreeMgr();

    bool Publish();
    bool Subscribe(std::string subs_addr, int port);

    void stop();
    std::shared_ptr<PropertyTreeHelper> GetData() { return this->data_;}
    void Broadcast();

    void Finish();

    std::vector<int> subs;
private:
    bool StartSubsServer(int port);
    void ListenConn();

    void ListenSync();
    std::shared_ptr<PropertyTreeHelper> data_;


    std::thread broadcast_thread_;
    std::thread sync_thread_;
    bool subscribe_server_running = false;
};

} // namespace PropertyTree


#endif //PROPERTYTREE_PROPERTY_TREE_H
