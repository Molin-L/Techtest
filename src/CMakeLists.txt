# Add all the source files in the folder to the library
file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h")

include_directories(
        ${PROJECT_SOURCE_DIR}/deps/flatbuffers-master/include
)

# Create the library
add_library(data_mgr ${SOURCE_FILES} datamgr.h datamgr.cpp datamgr.cpp)
