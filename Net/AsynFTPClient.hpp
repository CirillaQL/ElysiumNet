//
// Created by QianL on 2020/6/25.
//

#ifndef ELYSIUMNET_ASYNFTPCLIENT_HPP
#define ELYSIUMNET_ASYNFTPCLIENT_HPP

#include <windows.h>
#include <winsock2.h>
#include <iostream>

using namespace std;

class FTPClient{
private:
    WSADATA wsadata;
    SOCKET clientSocket;
    SOCKADDR_IN socketAddr;
    int connectStatus = SOCKET_ERROR;
    char sendBuf[8192];
public:
    //初始化
    FTPClient();
    //设置地址和端口
    void setAddressAndPort(const char* address, u_short port);
    //设置端口
    void setAddressAndPort(u_short port);
    //连接
    void connectToServer();
    //执行功能
    void run();
    //选择功能
    void selectOrder(string order);
};

FTPClient::FTPClient() {
    WORD wsa = MAKEWORD(2,2);
    if(WSAStartup(wsa,&wsadata) != 0){
        cout << "初始化失败" << endl;
        exit(1);
    }
    clientSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(clientSocket == INVALID_SOCKET){
        cout << "socket创建失败" << endl;
        exit(1);
    }
}

void FTPClient::setAddressAndPort(const char *address, u_short port) {
    memset(&socketAddr,0,sizeof(socketAddr));
    socketAddr.sin_family = AF_INET;
    socketAddr.sin_addr.s_addr = inet_addr(address);
    socketAddr.sin_port = htons(port);
}

void FTPClient::setAddressAndPort(u_short port) {
    memset(&socketAddr,0,sizeof(socketAddr));
    socketAddr.sin_family = AF_INET;
    socketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    socketAddr.sin_port = htons(port);
}

void FTPClient::connectToServer() {
    int count = 0;
    connectStatus = connect(clientSocket,(SOCKADDR *)&socketAddr,sizeof(SOCKADDR));
    while (count <= 10 && connectStatus == SOCKET_ERROR){
        cout << "正在尝试重新连接...." << endl;
        Sleep(1000);
        connectStatus = connect(clientSocket,(SOCKADDR *)&socketAddr,sizeof(SOCKADDR));
        count++;
    }
    if(connectStatus == SOCKET_ERROR){
        cout << "无法连接到服务器!" << endl;
        exit(1);
    } else{
        cout << "成功连接到服务器！" << endl;
        run();
    }
}

void FTPClient::run() {
    //Check Again
    if(connectStatus == SOCKET_ERROR){
        cout << "连接异常" << endl;
        exit(1);
    }

    //获取指令
    string option;
    cin >> option;
    while(option != "quit"){
        cout << "输入命令: " <<  option << endl;
        selectOrder(option);
        cin >> option;
    }
}

void FTPClient::selectOrder(string order) {
    if(order == "dir"){
        for (int i = 0; i < order.size(); i++){
            sendBuf[i] = order[i];
        }
        int res_len = send(clientSocket, sendBuf,sizeof(sendBuf),0);
        if(res_len < 0){
            cout << "发送失败" << endl;
        }

    }
}

#endif //ELYSIUMNET_ASYNFTPCLIENT_HPP
