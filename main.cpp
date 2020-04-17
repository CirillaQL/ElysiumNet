
#include "Net/SingleSocketServer.hpp"
#include "Net/AsynServer.hpp"

int main() {
    Server server;
    server.setPort(7865);
    server.Listen();
    server.begin();
    //std::cout << "Hello, World!" << std::endl;
    return 0;
}
