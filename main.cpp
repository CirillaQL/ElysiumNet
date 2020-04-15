
#include "Net/SingleSocketServer.hpp"


int main() {
    SingleServer server;
    server.setPort(7865);
    server.begin();
    //std::cout << "Hello, World!" << std::endl;
    return 0;
}
