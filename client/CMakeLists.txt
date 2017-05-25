cmake_minimum_required(VERSION 2.8)

project(client)
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
add_executable(client main.cpp UploadWorker.cpp FileSystemWatcher.cpp FileWatcher.cpp FileWatcherLinux.cpp)
target_link_libraries(client boost_system pthread)
