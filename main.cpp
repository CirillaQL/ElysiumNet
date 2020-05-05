
#include "Net/AsynFTPServer.hpp"

int main() {
    FTPServer a;
    a.setPort(9090);
    a.start();
    //std::cout << "Hello, World!" << std::endl;
    return 0;
}
