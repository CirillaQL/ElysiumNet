
#include "Net/AsynFTPServer.hpp"

int main() {
    FTPServer a;
    a.setPort(9090);
    a.start();


    return 0;
}
