cmake_minimum_required(VERSION 3.15)
project(ElysiumNet)

set(CMAKE_CXX_STANDARD 14)
link_libraries(ws2_32 wsock32)
add_executable(ElysiumNet main.cpp Net/SingleSocketServer.hpp Net/SingleSocketClient.hpp Net/AsynServer.hpp Net/Time.hpp Net/AsynFTPServer.hpp Net/AsynFTPClient.hpp)