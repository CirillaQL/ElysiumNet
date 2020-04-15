//
// Created by QianL on 2020/4/15.
//

#ifndef ELYSIUMNET_SINGLESOCKETCLIENT_HPP
#define ELYSIUMNET_SINGLESOCKETCLIENT_HPP

#include <winsock2.h>
#include <afxres.h>
#include <iostream>

using namespace std;

class SingleClient{
private:
    WSADATA wsadata;
    SOCKET socket_Client;
    struct sockaddr_in addr = {0};
public:
    SingleClient(){
        WSAStartup(MAKEWORD(2,2),&wsadata);
        SOCKET socket_Client = socket(AF_INET,SOCK_STREAM,0);
        if(socket_Client == INVALID_SOCKET)
        {
            cout << "客户端初始化失败" << endl;
        }
    }
    void setPort(unsigned short port){
        this->addr.sin_family = AF_INET;
        this->addr.sin_port = htons(port);
        this->addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    }
    void setIP(const char *ip_address){
        this->addr.sin_addr.S_un.S_addr = inet_addr(ip_address);
    }
    void begin(){
        int iRetVal = connect(socket_Client,(struct sockaddr*)&addr,sizeof(addr));
        if(SOCKET_ERROR == iRetVal)
        {
            printf("服务器连接失败！");
            closesocket(socket_Client);
        }
        printf("服务器连接成功！\n");
        //数据收发
        CHAR szSend[100] = "END";   //客户端  先发后收
        send(socket_Client,szSend,sizeof(szSend),0);  //发送函数，可以通过返回值判断发送成功与否。
        //接收服务器回传的数据
        CHAR szRecv[100] = {0};
        recv(socket_Client,szRecv,100,0); //接收函数
        printf("%s\n",szRecv);//表示以字符串的格式输出服务器端发送的内容。
    }
    void close(){
        closesocket(socket_Client);
    }
    ~SingleClient(){
        closesocket(socket_Client);
        WSACleanup();
    }
};

#endif //ELYSIUMNET_SINGLESOCKETCLIENT_HPP
