#include <fstream>
#include <string>
#include <iostream>
#include <memory>
#include <filesystem>
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"
#include "schema/property_generated.h"
#include "tests/utils.h"


std::string load_from_file() {
    std::filesystem::path cwd = std::filesystem::current_path();
    std::string data_path = cwd.string().append("/../tests/testdata.bin");

    std::ifstream test_data(data_path, std::ios::binary);
    if (test_data.is_open()) {
        std::vector<char> buffer((std::istreambuf_iterator<char>(test_data)), std::istreambuf_iterator<char>());
        return std::string(buffer.begin(), buffer.end());
    }
    return "";
}

void generate_sample_data() {
    // Create a FlatBufferBuilder instance
    flatbuffers::FlatBufferBuilder builder;

    auto sub_property_1_name = builder.CreateString("sub_property_1");
    auto sub_property_2_name = builder.CreateString("sub_property_2");
    auto sub_property_2_value = builder.CreateString("Hello, World!");
    auto property_name = builder.CreateString("property_name");
    // Create sub-properties
    std::vector<flatbuffers::Offset<PropertyTree::Property>> sub_properties;

    // Create first sub-property
    PropertyTree::PropertyValueBuilder int_value_builder(builder);
    int_value_builder.add_int_value(42);
    auto int_value = int_value_builder.Finish();

    PropertyTree::PropertyBuilder sub_property_builder1(builder);
    sub_property_builder1.add_name(sub_property_1_name);
    sub_property_builder1.add_type(PropertyTree::Type_INT);
    sub_property_builder1.add_value(int_value);
    auto sub_property1 = sub_property_builder1.Finish();
    sub_properties.push_back(sub_property1);

    // Create second sub-property
    PropertyTree::PropertyValueBuilder string_value_builder(builder);
    string_value_builder.add_string_value(sub_property_2_value);
    auto string_value = string_value_builder.Finish();

    PropertyTree::PropertyBuilder sub_property_builder2(builder);
    sub_property_builder2.add_name(sub_property_2_name);
    sub_property_builder2.add_type(PropertyTree::Type_STRING);
    sub_property_builder2.add_value(string_value);
    auto sub_property2 = sub_property_builder2.Finish();
    sub_properties.push_back(sub_property2);

    auto subproperties = builder.CreateVector(sub_properties);
    // Create Property
    PropertyTree::PropertyBuilder prop_builder(builder);
    prop_builder.add_name(property_name);
    prop_builder.add_type(PropertyTree::Type_INT);
    prop_builder.add_value(int_value);
    prop_builder.add_sub_properties(subproperties);
    auto property = prop_builder.Finish();

    builder.Finish(property);

    // Get the serialized data
    const uint8_t* buf = builder.GetBufferPointer();
    size_t size = builder.GetSize();

    // Export the serialized data to a file
    std::ofstream file("/Users/molin/Code/CryptoDotComTechTest/tests/testdata.bin", std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(buf), size);
        file.close();
        std::cout << "Data exported to file successfully." << std::endl;
    } else {
        std::cerr << "Error: Unable to open the file for writing." << std::endl;
    }
}