#include <iostream>
#include "Net/SingleSocketServer.hpp"
#include "Net/SingleSocketClient.hpp"

int main() {
    SingleServer server;
    server.Init();
    server.Bind(4999);
    server.Listen();
    server.begin();
    //std::cout << "Hello, World!" << std::endl;
    return 0;
}
