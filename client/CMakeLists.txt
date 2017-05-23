cmake_minimum_required(VERSION 2.8)

project(client)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
add_executable(client main.cpp UploadWorker.cpp FileSystemWatcher.cpp)
target_link_libraries(client boost_system pthread)
