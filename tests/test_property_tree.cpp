#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <thread>
#include "schema/property_generated.h"
#include "flatbuffers/reflection_generated.h"
#include "flatbuffers/util.h"
#include "flatbuffers/minireflect.h"
#include "flatbuffers/reflection.h"
#include "utils.h"
#include "tcp_service/tcp_conn.h"
#include "property_tree/property_tree.h"
#include "flatbuffers/idl.h"

TEST(Task2, ReadData) {
    auto property_char = load_from_file();
    auto property = PropertyTree::GetProperty(property_char.data());
    ASSERT_EQ(property->name()->str(), "property_name");
    ASSERT_EQ(property->value()->int_value(), 42);
    ASSERT_EQ(property->type(), PropertyTree::Type_INT);
    ASSERT_EQ(property->sub_properties()->size(), 2);
    ASSERT_EQ(property->sub_properties()->Get(0)->name()->str(), "sub_property_1");
    ASSERT_EQ(property->sub_properties()->Get(0)->value()->int_value(), 42);
    ASSERT_EQ(property->sub_properties()->Get(0)->type(), PropertyTree::Type_INT);
    ASSERT_EQ(property->sub_properties()->Get(1)->name()->str(), "sub_property_2");
    ASSERT_EQ(property->sub_properties()->Get(1)->value()->string_value()->str(), "Hello, World!");
    ASSERT_EQ(property->sub_properties()->Get(1)->type(), PropertyTree::Type_STRING);
}

TEST(Task2, UpdateData) {
    auto property_char = load_from_file();
    auto property = PropertyTree::GetMutableProperty(property_char.data());
    auto property_table = flatbuffers::GetRoot<PropertyTree::Property>(property_char.data());

    // Before modified
    ASSERT_EQ(property->name()->str(), "property_name");
    ASSERT_EQ(property->value()->int_value(), 42);
    ASSERT_EQ(property->type(), PropertyTree::Type_INT);
    ASSERT_EQ(property->sub_properties()->size(), 2);
    ASSERT_EQ(property->sub_properties()->Get(0)->name()->str(), "sub_property_1");
    ASSERT_EQ(property->sub_properties()->Get(0)->value()->int_value(), 42);
    ASSERT_EQ(property->sub_properties()->Get(0)->type(), PropertyTree::Type_INT);
    ASSERT_EQ(property->sub_properties()->Get(1)->name()->str(), "sub_property_2");
    ASSERT_EQ(property->sub_properties()->Get(1)->value()->string_value()->str(), "Hello, World!");
    ASSERT_EQ(property->sub_properties()->Get(1)->type(), PropertyTree::Type_STRING);


    // Modify
    // Scale data can be modified directly
    auto mutable_property = PropertyTree::GetMutableProperty(property_char.data());
    mutable_property->mutable_value()->mutate_int_value(11);
    ASSERT_EQ(mutable_property->value()->int_value(), 11);

    // As for string and vector, we need to use reflection to modify
    std::string bfbsfile;
    std::filesystem::path cwd = std::filesystem::current_path();
    std::string bfbsfile_path = cwd.string().append("/../schema/property.bfbs");
    ASSERT_TRUE(flatbuffers::LoadFile((bfbsfile_path).c_str(),
                                      true, &bfbsfile));
    flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t *>(bfbsfile.c_str()), bfbsfile.length());
    ASSERT_EQ(PropertyTree::VerifyPropertyBuffer(verifier), true);
    auto &schema = *reflection::GetSchema(bfbsfile.c_str());
    auto root_table = schema.root_table();

    std::vector<uint8_t> resizingbuf(property_char.begin(), property_char.end());

    auto &name_field = *root_table->fields()->LookupByKey("name");
    auto rroot = flatbuffers::piv(flatbuffers::GetAnyRoot(resizingbuf.data()),
                                  resizingbuf);

    SetString(schema, "property_name_new", GetFieldS(**rroot, name_field),
              &resizingbuf);

    auto new_property = PropertyTree::GetMutableProperty(resizingbuf.data());

    // After modified
    ASSERT_EQ(new_property->name()->str(), "property_name_new");
}

TEST(Task3, SendNReceiveOverTCP) {
    auto property_char = load_from_file();
    auto server = std::make_shared<TcpConn>(8085);
    server->start_server();
    auto client = std::make_shared<TcpConn>();
    ASSERT_TRUE(client->connect_to("127.0.0.1", 8085));
    ASSERT_TRUE(client->send_buffer(property_char.data(), property_char.size()));
}

TEST(Task4, ReflectionToRead) {
    auto property_char = load_from_file();
    auto server = std::make_shared<TcpConn>(8485);
    server->start_server_with_handler([](char *buffer, int len) {
        std::string bfbsfile;
        std::filesystem::path cwd = std::filesystem::current_path();
        std::string bfbsfile_path = cwd.string().append("/../schema/property.bfbs");
        ASSERT_TRUE(flatbuffers::LoadFile((bfbsfile_path).c_str(),
                                          true, &bfbsfile));
        flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t *>(bfbsfile.c_str()), bfbsfile.length());
        ASSERT_EQ(PropertyTree::VerifyPropertyBuffer(verifier), true);
        auto &schema = *reflection::GetSchema(bfbsfile.c_str());
        auto root_table = schema.root_table();

        auto obj = schema.objects();
        ASSERT_TRUE(root_table);

        ASSERT_EQ(root_table->name()->str(), "PropertyTree.Property");

        auto &root = *flatbuffers::GetAnyRoot(reinterpret_cast<const uint8_t *>(buffer));
        flatbuffers::FlatBufferBuilder schema_builder;

        auto name_field_ptr = root_table->fields()->LookupByKey("name");
        auto &name_field = *name_field_ptr;
        ASSERT_EQ(flatbuffers::GetFieldS(root, name_field)->str(), "property_name");

        auto value_field_ptr = root_table->fields()->LookupByKey("value");
        auto &value_field = *value_field_ptr;
        ASSERT_EQ(flatbuffers::GetFieldI<int>(root, value_field), 100);
    });
    auto client = std::make_shared<TcpConn>();
    ASSERT_TRUE(client->connect_to("127.0.0.1", 8485));
    ASSERT_TRUE(client->send_buffer(property_char.data(), property_char.size()));
}

TEST(Task5, UpdatePropertyViaSubscription) {
    auto test_data = load_from_file();
    // Initialize sender
    auto sender_helper = std::make_shared<PropertyTree::PropertyTreeHelper>(test_data.data(), test_data.size());
    auto sender = PropertyTree::PropertyTreeMgr(sender_helper);
    // Initialize receiver
    auto receiver = PropertyTree::PropertyTreeMgr();

    // Sender publish subscription
    ASSERT_TRUE(sender.Publish());
    // Receiver subscribe property
    ASSERT_TRUE(receiver.Subscribe("127.0.0.1", 8545));
    ASSERT_EQ(sender.subs.size(), 1);
    sleep(1);
    auto receiver_name = receiver.GetData()->GetProperty()->name()->str();
    ASSERT_EQ(receiver_name, "Sender changed name");

    sender.GetData()->SetName("Sender changed name");
    sender.Finish();
    receiver_name = receiver.GetData()->GetProperty()->name()->str();
    ASSERT_EQ(receiver_name, "Sender changed name");
}