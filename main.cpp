
#include "Net/AsynFTPServer.hpp"

int main() {
    FTPServer a;
    a.setPort(1234);
    a.start();
    //std::cout << "Hello, World!" << std::endl;
    return 0;
}
