//
// Created by QianL on 2020/4/16.
//

#ifndef ELYSIUMNET_ASYNSERVER_HPP
#define ELYSIUMNET_ASYNSERVER_HPP

#include <iostream>
#include <winsock2.h>
#include <process.h>
#include <mutex>
#include <deque>
#include <map>

unsigned int WINAPI CreateServer(LPVOID args);
unsigned int WINAPI Proc(LPVOID args);

using namespace std;

char buf[1024];
const int _bufLen = 1024;

struct AsynClient
{
    WSAOVERLAPPED overlapped;
    SOCKET socket;
    WSABUF buf;
    int procID;
    int id;
};

class Server{
private:
    SYSTEM_INFO  sysInfo;
    HANDLE CompPort;
    DWORD dwRecvCount = 0;
    DWORD nFlag = 0;
    map<int, AsynClient*> Clients;
    mutex _mutex;
    int _thread_count;
public:
    Server(){
        CompPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
        GetSystemInfo(&sysInfo);
        _thread_count = sysInfo.dwNumberOfProcessors * 2;
    }
    void release(AsynClient* c)
    {
        _mutex.lock();
        Clients.erase(c->id);
        _mutex.unlock();
        closesocket(c->socket);
        delete[] c->buf.buf;
        delete c;
    }
    void begin(){
        CreateServer(0);
        for (int i = 0; i < _thread_count; i++) {
            int* temp = new int(i);
            _beginthreadex(0, 0, reinterpret_cast<unsigned int (*)(void *)>(this->Proc(0)), temp, 0, 0);
        }
    }
    unsigned int WINAPI CreateServer(LPVOID args){
        WORD wVersion;
        WSADATA wsaData;
        int err;
        wVersion = MAKEWORD(2, 2);
        err = WSAStartup(wVersion, &wsaData);
        if (err != 0) {
            return 0;
        }
        if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
            WSACleanup();
            return 0;
        }
        SOCKET sockSrv = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
        const char chOpt = 1;
        setsockopt(sockSrv, IPPROTO_TCP, TCP_NODELAY, &chOpt, sizeof(chOpt));

        int nSendBufLen = 16 * 1024 * 1024;
        setsockopt(sockSrv, SOL_SOCKET, SO_SNDBUF, (const char*)&nSendBufLen, sizeof(int));

        SOCKADDR_IN addrSrv;
        addrSrv.sin_addr.S_un.S_addr = htonl(ADDR_ANY);
        addrSrv.sin_family = AF_INET;
        addrSrv.sin_port = htons(6001);
        ::bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
        err = listen(sockSrv, SOMAXCONN);
        if (err == SOCKET_ERROR) {
            cout << "listen failed" << endl;
            WSACleanup();
            return 0;
        }
        SOCKADDR_IN remoteAddr;
        int addrSize = sizeof(remoteAddr);
        //accept loop
        while (true) {
            SOCKET s = accept(sockSrv, (SOCKADDR*)&remoteAddr, &addrSize);;
            AsynClient* c = new AsynClient;
            memset(c, 0, sizeof(AsynClient));
            c->socket = s;
            char*  buf = new char[_bufLen];
            memset(buf, 0, _bufLen);
            c->buf.buf = buf;
            c->buf.len = _bufLen;
            _mutex.lock();
            int id;
            do
            {
                id = rand() % INT_MAX;
            } while (Clients.find(id) != Clients.end());
            Clients.insert(pair<int, AsynClient*>(id, c));
            c->id = id;
            _mutex.unlock();
            if(CreateIoCompletionPort((HANDLE)c->socket,CompPort,(ULONG_PTR)c,0)==0)
            {
                continue;
            }
            if(WSARecv(c->socket, &c->buf, 1, &dwRecvCount, &nFlag, &c->overlapped, 0)==SOCKET_ERROR)
            {
                int err = WSAGetLastError();
                if(err!=WSA_IO_PENDING)
                {
                    release(c);
                }
            }
        }
        return 0;
    }
    unsigned int WINAPI Proc(LPVOID args)
    {
        while (true)
        {
            AsynClient* c;
            DWORD dwTransferred;
            LPWSAOVERLAPPED overlapped;
            if(GetQueuedCompletionStatus(CompPort, &dwTransferred, (PULONG_PTR)&c, &overlapped,INFINITE))
            {
                if(dwTransferred==0)
                {
                    release(c);
                    continue;
                }
                cout << buf << endl;
                memset(c->buf.buf, 0, _bufLen);
                const char* l = "shit";
                send(c->socket, l, 128, 0);

                if (WSARecv(c->socket, &c->buf, 1, &dwRecvCount, &nFlag, &c->overlapped, 0) == SOCKET_ERROR)
                {
                    int err = WSAGetLastError();
                    if (err != WSA_IO_PENDING)
                    {
                        release(c);
                    }
                }
            }
            else
            {
                release(c);
            }
        }
    }
};



#endif //ELYSIUMNET_ASYNSERVER_HPP
