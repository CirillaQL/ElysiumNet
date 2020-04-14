//
// Created by QianL on 2020/4/10.
//

#ifndef ELYSIUMNET_SINGLESOCKETSERVER_HPP
#define ELYSIUMNET_SINGLESOCKETSERVER_HPP

#include <winsock2.h>
#include <iostream>

const int BUF_SIZE = 1024;

using namespace std;

class SingleServer{
private:
    WSADATA wsadata;
    SOCKET socket_Server;
    SOCKET socket_Client;
    SOCKADDR_IN addrServer;
    char rev_buf[BUF_SIZE];
    char send_buf[BUF_SIZE];
    int retVal;
public:
    SingleServer()= default;

    bool Init(){
        if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
        {
            cout << "WSAStartup failed!" << endl;
            return false;
        }
        //创建套接字
        socket_Server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (INVALID_SOCKET == socket_Server)
        {
            cout << "socket failed!" << endl;
            WSACleanup();//释放套接字资源;
            return false;
        }
        return true;
    }

    bool Bind(unsigned short port){
        addrServer.sin_family = AF_INET;
        addrServer.sin_port = htons(port);
        addrServer.sin_addr.s_addr = INADDR_ANY;
        //绑定套接字
        retVal = bind(socket_Server, (LPSOCKADDR)&addrServer, sizeof(SOCKADDR_IN));
        if (SOCKET_ERROR == retVal)
        {
            cout << "bind failed!" << endl;
            closesocket(socket_Server);   //关闭套接字
            WSACleanup();           //释放套接字资源;
            return false;
        }
        return true;
    }

    bool Listen(){
        retVal = listen(socket_Server, 1);
        if (SOCKET_ERROR == retVal)
        {
            cout << "listen failed!" << endl;
            closesocket(socket_Server);   //关闭套接字
            WSACleanup();           //释放套接字资源;
            return false;
        }
        return true;
    }

    void begin(){
        sockaddr_in addrClient;
        int addrClientlen = sizeof(addrClient);
        socket_Client = accept(socket_Server, (sockaddr FAR*)&addrClient, &addrClientlen);
        if (INVALID_SOCKET == socket_Client)
        {
            cout << "accept failed!" << endl;
            closesocket(socket_Server);   //关闭套接字
            WSACleanup();           //释放套接字资源;

        }

        while (true)
        {
            //接收客户端数据
            ZeroMemory(rev_buf, BUF_SIZE);
            retVal = recv(socket_Client, rev_buf, BUF_SIZE, 0);
            if (SOCKET_ERROR == retVal)
            {
                cout << "recv failed!" << endl;
                closesocket(socket_Server);   //关闭套接字
                closesocket(socket_Client);   //关闭套接字
                WSACleanup();           //释放套接字资源;
            }
            if (rev_buf[0] == '0')
                break;
            cout << "客户端发送的数据: " << rev_buf << endl;

            cout << "向客户端发送数据: ";
            cin >> send_buf;

            send(socket_Client, send_buf, strlen(send_buf), 0);
        }
    }

    void close(){
        closesocket(socket_Server);   //关闭套接字
        closesocket(socket_Client);   //关闭套接字
        WSACleanup();           //释放套接字资源;
    }
};

#endif //ELYSIUMNET_SINGLESOCKETSERVER_HPP
