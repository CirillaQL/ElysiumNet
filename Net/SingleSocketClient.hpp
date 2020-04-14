//
// Created by QianL on 2020/4/13.
//

#ifndef ELYSIUMNET_SINGLESOCKETCLIENT_HPP
#define ELYSIUMNET_SINGLESOCKETCLIENT_HPP

#include <winsock2.h>
#include <iostream>

using namespace std;

BOOL RecvLine(SOCKET s, char* buf);

class SingleClient{
private:
    WSADATA wsd; //WSADATA变量
    SOCKET sHost; //服务器套接字
    SOCKADDR_IN servAddr; //服务器地址
    char buf[BUF_SIZE]; //接收数据缓冲区
    char bufRecv[BUF_SIZE];
    int retVal; //返回值
public:
    SingleClient()= default;
    bool Init(){
        if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
        {
            cout << "WSAStartup failed!" << endl;
            return false;
        }
        //创建套接字
        sHost = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (INVALID_SOCKET == sHost)
        {
            cout << "socket failed!" << endl;
            WSACleanup();//释放套接字资源
            return false;
        }
    }
    bool setAddr(){
        servAddr.sin_family = AF_INET;
        servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        servAddr.sin_port = htons((short)7980);
        int nServAddlen = sizeof(servAddr);
    }
    bool setAddr(char* address, unsigned short port){
        servAddr.sin_family = AF_INET;
        servAddr.sin_addr.s_addr = inet_addr(address);
        servAddr.sin_port = htons(port);
        int nServAddlen = sizeof(servAddr);
    }
    bool begin(){
        retVal = connect(sHost, (LPSOCKADDR)&servAddr, sizeof(servAddr));
        if (SOCKET_ERROR == retVal)
        {
            cout << "connect failed!" << endl;
            closesocket(sHost); //关闭套接字
            WSACleanup(); //释放套接字资源
            return -1;
        }
        while (true)
        {
            //向服务器发送数据
            ZeroMemory(buf, BUF_SIZE);
            cout << " 向服务器发送数据:  ";
            cin >> buf;
            retVal = send(sHost, buf, strlen(buf), 0);
            if (SOCKET_ERROR == retVal)
            {
                cout << "send failed!" << endl;
                closesocket(sHost); //关闭套接字
                WSACleanup(); //释放套接字资源
                return -1;
            }
            //RecvLine(sHost, bufRecv);
            ZeroMemory(bufRecv, BUF_SIZE);
            recv(sHost, bufRecv, BUF_SIZE, 0); // 接收服务器端的数据， 只接收5个字符
            cout << endl << "从服务器接收数据：" << bufRecv;
            cout << "\n";
        }
    }
    void close(){
        closesocket(sHost); //关闭套接字
        WSACleanup(); //释放套接字资源
    }
};

#endif //ELYSIUMNET_SINGLESOCKETCLIENT_HPP
