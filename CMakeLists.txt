cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(ptask VERSION 0.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-std=c++14 -Wno-deprecated-register)

include_directories(/Users/anders/vfx/platform/2015_libcpp/include)
link_directories(/Users/anders/vfx/platform/2015_libcpp/lib)

add_executable(send send.cpp exr.cpp)
target_link_libraries(send zmq -lIlmImf -lHalf)
add_executable(sink sink.cpp exr.cpp)
target_link_libraries(sink zmq -lIlmImf -lHalf)
