//
// Created by QianL on 2020/4/16.
//

#ifndef ELYSIUMNET_ASYNSERVER_HPP
#define ELYSIUMNET_ASYNSERVER_HPP

#include <winsock2.h>
#include <iostream>
#include <thread>
#include "Time.hpp"

using namespace std;

#define BUF_SIZE 1024

void CALLBACK ReadCompRoutine(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
void CALLBACK WriteCompRoutine(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
void ErrorHanding(char * message);

typedef struct {
    SOCKET hClntSock;    //套接字句柄
    char buf[BUF_SIZE];
    WSABUF wsabuf;
}PER_IO_DATA,*LPPER_IO_DATA;

class Server{
private:
    WSAData wsaData;
    SOCKET hLinsnSock, hRecvSock;
    SOCKADDR_IN lisnAdr, recvAdr;
    LPWSAOVERLAPPED lpOvlap;
    DWORD recvBytes;
    LPPER_IO_DATA hbInfo;
    int recvAdrSz;
    DWORD flagInfo = 0;
    u_long mode = 1;
public:
    Server(){
        WORD _word = MAKEWORD(2,2);
        if(WSAStartup(_word,&this->wsaData) != 0){
            cout << "Socket API初始化失败. "<<endl;
        }
        hLinsnSock = WSASocket(PF_INET,SOCK_STREAM,0,NULL,0,WSA_FLAG_OVERLAPPED);
        //将hLisnSock句柄的套接字I/O模式(FIONBIO)改为mode中指定的非阻塞模式
        ioctlsocket(hLinsnSock,FIONBIO,&mode);
    }
    void setPort(const unsigned short port){
        memset(&lisnAdr,0,sizeof(lisnAdr));
        lisnAdr.sin_family = AF_INET;
        lisnAdr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
        lisnAdr.sin_port = htons(port);
    }
    //设置监听
    void Listen(){
        if (bind(hLinsnSock, (SOCKADDR*)&lisnAdr, sizeof(lisnAdr)) == SOCKET_ERROR)
            ErrorHanding("socket bind error!");
        if (listen(hLinsnSock, 5) == SOCKET_ERROR)
            ErrorHanding("socket listen error!");
    }
    void begin(){
        recvAdrSz = sizeof(recvAdr);
        while (1)
        {
            //进入短暂alertable wait 模式，运行ReadCompRoutine、WriteCompRoutine函数
            SleepEx(1, TRUE);

            //非阻塞套接字，需要处理INVALID_SOCKET
            //返回的新的套接字也是非阻塞的
            hRecvSock = accept(hLinsnSock, (SOCKADDR*)&recvAdr, &recvAdrSz);
            if (hRecvSock == INVALID_SOCKET)
            {
                //无客户端连接时，accept返回INVALID_SOCKET，WSAGetLastError()返回WSAEWOULDBLOCK
                if (WSAGetLastError() == WSAEWOULDBLOCK)
                    continue;
                else
                    ErrorHanding("accept() error");
            }

            puts("Client connected");

            //申请重叠I/O需要使用的结构体变量的内存空间并初始化
            //在循环内部申请：每个客户端需要独立的WSAOVERLAPPED结构体变量
            lpOvlap = (LPWSAOVERLAPPED)malloc(sizeof(WSAOVERLAPPED));
            memset(lpOvlap, 0, sizeof(WSAOVERLAPPED));

            hbInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
            hbInfo->hClntSock = (DWORD)hRecvSock;

            (hbInfo->wsabuf).buf = hbInfo->buf;
            (hbInfo->wsabuf).len = BUF_SIZE;

            //基于CR的重叠I/O不需要事件对象，故可以用来传递其他信息
            lpOvlap->hEvent = (HANDLE)hbInfo;
            //接收第一条信息
            WSARecv(hRecvSock, &(hbInfo->wsabuf), 1, &recvBytes, &flagInfo, lpOvlap, ReadCompRoutine);
        }
    }
};

void CALLBACK ReadCompRoutine(DWORD dwError, DWORD szRecvBytes, LPWSAOVERLAPPED lpOverlapped, DWORD flags)
{
    //从lpoverlapped中恢复传递的信息
    LPPER_IO_DATA hbInfo = (LPPER_IO_DATA)(lpOverlapped->hEvent);
    SOCKET hSock = hbInfo->hClntSock;
    LPWSABUF bufInfo = &(hbInfo->wsabuf);
    DWORD sentBytes;
    //接收到EOF，断开连接
    if (szRecvBytes == 0)
    {
        closesocket(hSock);
        free(hbInfo);
        free(lpOverlapped);
        puts("Client disconnected");
    }
    else
    {
        //bufInfo->len = szRecvBytes;
        string The_time = GetNowTime();
        char _time[100];
        int i = 0;
        for(;i < The_time.length();i++){
            _time[i] = The_time[i];
        }
        _time[i] = '\0';
        bufInfo->len = sizeof(_time);
        bufInfo->buf = _time;
        //将接收到的信息回传回去，传递完毕执行WriteCompRoutine(): 接收信息
        WSASend(hSock, bufInfo, 1, &sentBytes, 0, lpOverlapped, WriteCompRoutine);
    }
}

void CALLBACK WriteCompRoutine(DWORD dwError, DWORD szRecvBytes, LPWSAOVERLAPPED lpOverlapped, DWORD flags)
{
    //从lpoverlapped中恢复传递的信息
    LPPER_IO_DATA hbInfo = (LPPER_IO_DATA)(lpOverlapped->hEvent);
    SOCKET hSock = hbInfo->hClntSock;
    LPWSABUF bufInfo = &(hbInfo->wsabuf);
    DWORD recvBytes;
    DWORD flagInfo = 0;
    //接收数据，接收完毕执行ReadCompRoutine：发送数据
    WSARecv(hSock, bufInfo, 1, &recvBytes, &flagInfo, lpOverlapped, ReadCompRoutine);
    //cout << bufInfo->buf << endl;
}

void ErrorHanding(char * message)
{
    cout << message << endl;
    exit(1);
}

#endif //ELYSIUMNET_ASYNSERVER_HPP
