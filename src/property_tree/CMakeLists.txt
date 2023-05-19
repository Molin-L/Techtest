file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h")


# Add library
add_library(PropertyTree ${SOURCE_FILES} property_tree.cpp property_tree.h)

include_directories(
        ${PROJECT_SOURCE_DIR}/deps/flatbuffers-master/include
        ${PROJECT_SOURCE_DIR}/schema
)

target_include_directories(
        PropertyTree PUBLIC ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src
)


# Link the executable to the FlatBuffers library and googletest
target_link_libraries(PropertyTree PRIVATE flatbuffers tcp_service)