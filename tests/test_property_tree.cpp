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
#include "tcp_server.h"
#include "tcp_client.h"
#include "flatbuffers/idl.h"

TEST(Task2, ReadData) {
    auto property_char = load_from_file();
    auto property = PropertyTree::GetProperty(property_char.data());
    ASSERT_EQ(property->name()->str(), "property_name");
    ASSERT_EQ(property->value()->int_value(), 42);
    ASSERT_EQ(property->type(),  PropertyTree::Type_INT);
    ASSERT_EQ(property->sub_properties()->size(), 2);
    ASSERT_EQ(property->sub_properties()->Get(0)->name()->str(), "sub_property_1");
    ASSERT_EQ(property->sub_properties()->Get(0)->value()->int_value(), 42);
    ASSERT_EQ(property->sub_properties()->Get(0)->type(),  PropertyTree::Type_INT);
    ASSERT_EQ(property->sub_properties()->Get(1)->name()->str(), "sub_property_2");
    ASSERT_EQ(property->sub_properties()->Get(1)->value()->string_value()->str(), "Hello, World!");
    ASSERT_EQ(property->sub_properties()->Get(1)->type(),  PropertyTree::Type_STRING);
}

TEST(Task2, UpdateData) {
    auto property_char = load_from_file();
    auto property = PropertyTree::GetMutableProperty(property_char.data());
    auto property_table = flatbuffers::GetRoot<PropertyTree::Property>(property_char.data());

    // Before modified
    ASSERT_EQ(property->name()->str(), "property_name");


    flatbuffers::FlatBufferBuilder builder;
    std::string new_name = "new name new name new name new name";
    auto new_name_offset = builder.CreateString(new_name);
    PropertyTree::PropertyBuilder my_property_builder(builder);
    my_property_builder.add_name(new_name_offset);
    auto new_property_table = my_property_builder.Finish();
    builder.Finish(new_property_table);
    auto new_property = PropertyTree::GetMutableProperty(builder.GetBufferPointer());

    // After modified
    ASSERT_EQ(new_property->name()->str(), new_name);
}

TEST(Task3, SendNReceiveOverTCP) {
    auto property_char = load_from_file();
    auto server = std::make_shared<TcpServer>(8085);
    server->start_server();
    auto client = std::make_shared<TcpClient>();
    ASSERT_TRUE(client->connect_to_server("127.0.0.1", 8085));
    ASSERT_TRUE(client->send_buffer(property_char.data(),  property_char.size()));
}

TEST(Task4, ReflectionToRead) {
    auto property_char = load_from_file();
    auto server = std::make_shared<TcpServer>(8085);
    server->start_server_with_handler([](char* buffer, int len) {
        std::string bfbsfile;
        std::filesystem::path cwd = std::filesystem::current_path();
        std::string bfbsfile_path = cwd.string().append("/../schema/property.bfbs");
        ASSERT_TRUE(flatbuffers::LoadFile((bfbsfile_path).c_str(),
                                          true, &bfbsfile));
        flatbuffers::Verifier  verifier(reinterpret_cast<const uint8_t*>(bfbsfile.c_str()), bfbsfile.length());
        ASSERT_EQ(PropertyTree::VerifyPropertyBuffer(verifier), true);
        auto &schema = *reflection::GetSchema(bfbsfile.c_str());
        auto root_table = schema.root_table();

        auto obj = schema.objects();
        ASSERT_TRUE(root_table);

        ASSERT_EQ(root_table->name()->str(), "PropertyTree.Property");

        auto &root = *flatbuffers::GetAnyRoot(reinterpret_cast<const uint8_t*>(buffer));
        flatbuffers::FlatBufferBuilder schema_builder;

        auto name_field_ptr = root_table->fields()->LookupByKey("name");
        auto &name_field = *name_field_ptr;
        ASSERT_EQ(flatbuffers::GetFieldS(root, name_field)->str(), "property_name");

        auto value_field_ptr = root_table->fields()->LookupByKey("value");
        auto &value_field = *value_field_ptr;
        ASSERT_EQ(flatbuffers::GetFieldI<int>(root, value_field), 100);
    });
    auto client = std::make_shared<TcpClient>();
    ASSERT_TRUE(client->connect_to_server("127.0.0.1", 8085));
    ASSERT_TRUE(client->send_buffer(property_char.data(),  property_char.size()));
}
