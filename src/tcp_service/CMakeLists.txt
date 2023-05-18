# Add all the source files in the folder to the library
file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h")

include_directories(
        ../../deps/flatbuffers-master/include
        ../..
)

# Create the library
add_library(tcp_service ${SOURCE_FILES} tcp_server.h tcp_server.cpp tcp_client.cpp tcp_client.h ../property_tree/property_tree.cpp ../property_tree/property_tree.h)
