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
    SOCKET hClntSock;    //�׽��־��
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
            cout << "Socket API��ʼ��ʧ��. "<<endl;
        }
        hLinsnSock = WSASocket(PF_INET,SOCK_STREAM,0,NULL,0,WSA_FLAG_OVERLAPPED);
        //��hLisnSock������׽���I/Oģʽ(FIONBIO)��Ϊmode��ָ���ķ�����ģʽ
        ioctlsocket(hLinsnSock,FIONBIO,&mode);
    }
    void setPort(const unsigned short port){
        memset(&lisnAdr,0,sizeof(lisnAdr));
        lisnAdr.sin_family = AF_INET;
        lisnAdr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
        lisnAdr.sin_port = htons(port);
    }
    //���ü���
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
            //�������alertable wait ģʽ������ReadCompRoutine��WriteCompRoutine����
            SleepEx(1, TRUE);

            //�������׽��֣���Ҫ����INVALID_SOCKET
            //���ص��µ��׽���Ҳ�Ƿ�������
            hRecvSock = accept(hLinsnSock, (SOCKADDR*)&recvAdr, &recvAdrSz);
            if (hRecvSock == INVALID_SOCKET)
            {
                //�޿ͻ�������ʱ��accept����INVALID_SOCKET��WSAGetLastError()����WSAEWOULDBLOCK
                if (WSAGetLastError() == WSAEWOULDBLOCK)
                    continue;
                else
                    ErrorHanding("accept() error");
            }

            puts("Client connected");

            //�����ص�I/O��Ҫʹ�õĽṹ��������ڴ�ռ䲢��ʼ��
            //��ѭ���ڲ����룺ÿ���ͻ�����Ҫ������WSAOVERLAPPED�ṹ�����
            lpOvlap = (LPWSAOVERLAPPED)malloc(sizeof(WSAOVERLAPPED));
            memset(lpOvlap, 0, sizeof(WSAOVERLAPPED));

            hbInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
            hbInfo->hClntSock = (DWORD)hRecvSock;

            (hbInfo->wsabuf).buf = hbInfo->buf;
            (hbInfo->wsabuf).len = BUF_SIZE;

            //����CR���ص�I/O����Ҫ�¼����󣬹ʿ�����������������Ϣ
            lpOvlap->hEvent = (HANDLE)hbInfo;
            //���յ�һ����Ϣ
            WSARecv(hRecvSock, &(hbInfo->wsabuf), 1, &recvBytes, &flagInfo, lpOvlap, ReadCompRoutine);
        }
    }
};

void CALLBACK ReadCompRoutine(DWORD dwError, DWORD szRecvBytes, LPWSAOVERLAPPED lpOverlapped, DWORD flags)
{
    //��lpoverlapped�лָ����ݵ���Ϣ
    LPPER_IO_DATA hbInfo = (LPPER_IO_DATA)(lpOverlapped->hEvent);
    SOCKET hSock = hbInfo->hClntSock;
    LPWSABUF bufInfo = &(hbInfo->wsabuf);
    DWORD sentBytes;
    //���յ�EOF���Ͽ�����
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
        //�����յ�����Ϣ�ش���ȥ���������ִ��WriteCompRoutine(): ������Ϣ
        WSASend(hSock, bufInfo, 1, &sentBytes, 0, lpOverlapped, WriteCompRoutine);
    }
}

void CALLBACK WriteCompRoutine(DWORD dwError, DWORD szRecvBytes, LPWSAOVERLAPPED lpOverlapped, DWORD flags)
{
    //��lpoverlapped�лָ����ݵ���Ϣ
    LPPER_IO_DATA hbInfo = (LPPER_IO_DATA)(lpOverlapped->hEvent);
    SOCKET hSock = hbInfo->hClntSock;
    LPWSABUF bufInfo = &(hbInfo->wsabuf);
    DWORD recvBytes;
    DWORD flagInfo = 0;
    //�������ݣ��������ִ��ReadCompRoutine����������
    WSARecv(hSock, bufInfo, 1, &recvBytes, &flagInfo, lpOverlapped, ReadCompRoutine);
    //cout << bufInfo->buf << endl;
}

void ErrorHanding(char * message)
{
    cout << message << endl;
    exit(1);
}

#endif //ELYSIUMNET_ASYNSERVER_HPP
