#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "schema/property_generated.h"
#include "flatbuffers/reflection.h"
#include "utils.h"

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