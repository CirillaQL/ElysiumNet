//
// Created by QianL on 2020/5/2.
//

#ifndef ELYSIUMNET_ASYNFTPSERVER_HPP
#define ELYSIUMNET_ASYNFTPSERVER_HPP

#include <windows.h>
#include <winsock2.h>
#include <iostream>
#include <process.h>

using namespace std;

#define BUF_SIZE 8192

class SystemInfo{
private:
    SYSTEM_INFO systemInfo{};
    int NumberOfCore = 0;
public:
    SystemInfo(){
        GetSystemInfo(&systemInfo);
        NumberOfCore = systemInfo.dwNumberOfProcessors;
    }
    int get_Number_Processors() const{
        return NumberOfCore;
    };
    ~SystemInfo()= default;
};

typedef struct{
    SOCKET Client;
    SOCKADDR_IN Client_Addr;
} HANDLE_DATA, *LPPER_HANDLE_DATA;

typedef struct    // buffer info
{
    OVERLAPPED overlapped;
    WSABUF wsaBuf;
    char buffer[BUF_SIZE];
    int rwMode;    // READ or WRITE 读写模式
} PER_IO_DATA, *LPPER_IO_DATA;

unsigned int WINAPI WorkerThread(LPVOID CompletionPortIO);//工作线程
SOCKET ALLCLIENT[1024];
int client_count = 0;
HANDLE mutex;

class FTPServer{
private:
    WSADATA wsadata;
    HANDLE CompletionPort;      //完成端口
    LPPER_IO_DATA ioINFO;
    LPPER_HANDLE_DATA handleInfo;

    SOCKET ServerSocket;
    SOCKADDR_IN ServerAddr;
    
    DWORD recvBytes = 0, flag = 0;
public:
    FTPServer(){
        if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0){
            cout << "初始化失败" << endl;
        }
        CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr,0,0);
        auto a = new SystemInfo();
        for (int i = 0; i < a->get_Number_Processors(); i++) {
            _beginthreadex(nullptr, 0, WorkerThread, (LPVOID)CompletionPort, 0, nullptr);
        }
        ServerSocket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);//不是非阻塞套接字，但是重叠IO套接字。
    }
    void setPort(short port){
        memset(&ServerAddr, 0, sizeof(ServerAddr));
        ServerAddr.sin_family = AF_INET;
        ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        ServerAddr.sin_port = htons(port);
        
        bind(ServerSocket,(SOCKADDR*)&ServerAddr,sizeof(ServerAddr));
        listen(ServerSocket,10);
    }

    void start(){
        while(true){
            SOCKET hClntSock;
            SOCKADDR_IN clntAdr;
            int addrLen = sizeof(clntAdr);

            hClntSock = accept(ServerSocket, (SOCKADDR*)&clntAdr, &addrLen);

            handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(HANDLE_DATA));//和重叠IO一样
            handleInfo->Client = hClntSock;//存储客户端套接字
            //WaitForSingleObject为等待线程都完毕后再执行。
            WaitForSingleObject(mutex, INFINITE);//线程同步

            ALLCLIENT[client_count++] = hClntSock;//存入套接字队列

            ReleaseMutex(mutex);

            memcpy(&(handleInfo->Client_Addr), &clntAdr, addrLen);

            CreateIoCompletionPort((HANDLE)hClntSock, CompletionPort, reinterpret_cast<ULONG_PTR>(handleInfo), 0);//连接套接字和CP对象
            //已完成信息将写入CP对象
            ioINFO = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));//存储接收到的信息
            memset(&(ioINFO->overlapped), 0, sizeof(OVERLAPPED));
            ioINFO->wsaBuf.len = BUF_SIZE;
            ioINFO->wsaBuf.buf = ioINFO->buffer;//和重叠IO一样
            ioINFO->rwMode = 1;//读写模式

            WSARecv(handleInfo->Client, &(ioINFO->wsaBuf),//非阻塞模式
                    1, &recvBytes, &flag, &(ioINFO->overlapped), nullptr);
        }
    }
};

unsigned int WINAPI WorkerThread(LPVOID CompletionPortIO)//线程的执行
{
    HANDLE hComPort = (HANDLE)CompletionPortIO;
    SOCKET sock;
    DWORD bytesTrans;
    LPPER_HANDLE_DATA handleInfo;
    LPPER_IO_DATA ioInfo;
    DWORD flags = 0;

    while (1)//大循环
    {
        GetQueuedCompletionStatus(hComPort, &bytesTrans,//确认“已完成”的I/O！！
                                  reinterpret_cast<PULONG_PTR>((LPDWORD) &handleInfo), (LPOVERLAPPED*)&ioInfo, INFINITE);//INFINITE使用时，程序将阻塞，直到已完成的I/O信息写入CP对象
        sock = handleInfo->Client;//客户端套接字

        if (ioInfo->rwMode == 1)//读写模式（此时缓冲区有数据）
        {
            puts("message received!");
            if (bytesTrans == 0)    // 连接结束
            {
                WaitForSingleObject(mutex, INFINITE);//线程同步

                closesocket(sock);
                int i = 0;
                while (ALLCLIENT[i] == sock){ i++; }
                ALLCLIENT[i] = 0;//断开置0

                ReleaseMutex(mutex);

                free(handleInfo); free(ioInfo);
                continue;
            }
            int i = 0;

            for (; i < client_count;i++)
            {
                if (ALLCLIENT[i] != 0)//判断是否为已连接的套接字
                {
                    if (ALLCLIENT[i] != sock)
                    {
                        LPPER_IO_DATA newioInfo;
                        newioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));//动态分配内存
                        memset(&(newioInfo->overlapped), 0, sizeof(OVERLAPPED));
                        strcpy(newioInfo->buffer, ioInfo->buffer);//重新构建新的内存，防止多次释放free
                        newioInfo->wsaBuf.buf = newioInfo->buffer;
                        newioInfo->wsaBuf.len = bytesTrans;
                        newioInfo->rwMode = 0;

                        WSASend(ALLCLIENT[i], &(newioInfo->wsaBuf),//回声
                                1, NULL, 0, &(newioInfo->overlapped), NULL);
                    }
                    else
                    {
                        memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
                        ioInfo->wsaBuf.len = bytesTrans;
                        ioInfo->rwMode = 0;
                        WSASend(ALLCLIENT[i], &(ioInfo->wsaBuf),//回声
                                1, NULL, 0, &(ioInfo->overlapped), NULL);
                    }
                }
            }
            ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));//动态分配内存
            memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
            ioInfo->wsaBuf.len = BUF_SIZE;
            ioInfo->wsaBuf.buf = ioInfo->buffer;
            ioInfo->rwMode = 1;
            WSARecv(sock, &(ioInfo->wsaBuf),//再非阻塞式接收
                    1, NULL, &flags, &(ioInfo->overlapped), NULL);
        }
        else
        {
            puts("message sent!");
            free(ioInfo);
        }
    }
    return 0;
}

#endif //ELYSIUMNET_ASYNFTPSERVER_HPP
