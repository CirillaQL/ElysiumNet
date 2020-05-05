//
// Created by QianL on 2020/5/2.
//

#ifndef ELYSIUMNET_ASYNFTPSERVER_HPP
#define ELYSIUMNET_ASYNFTPSERVER_HPP

#include <windows.h>
#include <winsock2.h>
#include <iostream>
#include <stdio.h>
#include <memory.h>

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

typedef struct _PER_HANDLE_DATA
{
    SOCKET sock;
}PER_HANDLE_DATA,* LPPER_HANDLE_DATA;

typedef struct _PER_IO_OPERATION_DATA
{
    OVERLAPPED Overlapped;
    WSABUF DataBuff;
    char Buff[BUF_SIZE];
    //BOOL OperationType;
}PER_IO_OPERATION_DATA,* LPPER_IO_OPERATION_DATA;

unsigned int WINAPI WorkerThread(LPVOID CompletionPort);//工作线程

class FTPServer{
private:
    WSADATA wsadata;
    DWORD nThreadID;
    SOCKET sockListen;
    struct sockaddr_in addrLocal;
    int nReuseAddr = 1;
    LPPER_HANDLE_DATA perHandleData;
    LPPER_IO_OPERATION_DATA ioperdata;
    SOCKET sockAccept;
    DWORD dwFlags = 0;
    DWORD dwRecvBytes;
    HANDLE hCompletionPort;
public:
    FTPServer(){
        if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0){
            cout << "初始化失败" << endl;
        }
        hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr,0,0);
        auto a = new SystemInfo();
        for (int i = 1; i <= a->get_Number_Processors() * 2; i++) {
            _beginthreadex(nullptr, 0, WorkerThread, (LPVOID)hCompletionPort, 0, nullptr);
            cout<<"创建工作者线程"<<i<<endl;
        }
        sockListen = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);//不是非阻塞套接字，但是重叠IO套接字。
        if(setsockopt(sockListen,SOL_SOCKET,SO_REUSEADDR,(const char *)&nReuseAddr,sizeof(int)) != 0)
        {
            cout<<"setsockopt错误"<<endl;
            exit(1);
        }
    }
    void setPort(short port){
        memset(&addrLocal, 0, sizeof(addrLocal));
        addrLocal.sin_family = AF_INET;
        addrLocal.sin_addr.s_addr = htonl(INADDR_ANY);
        addrLocal.sin_port = htons(port);
        
        bind(sockListen,(SOCKADDR*)&addrLocal,sizeof(addrLocal));
        listen(sockListen,10);
        cout<<"Socket Listen 监听中..."<<endl;
    }

    void start(){
        while(true){
            sockAccept = WSAAccept(sockListen,NULL,NULL,NULL,0);
            perHandleData = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
            if(perHandleData == NULL)
                continue;
            cout<<"Accept:socket number "<<sockAccept<<"接入"<<endl;
            perHandleData->sock = sockAccept;

            ioperdata = (LPPER_IO_OPERATION_DATA)malloc(sizeof(PER_IO_OPERATION_DATA));
            memset(&(ioperdata->Overlapped),0,sizeof(OVERLAPPED));
            ioperdata->DataBuff.len = BUF_SIZE;
            ioperdata->DataBuff.buf = ioperdata->Buff;
            if( ioperdata == NULL)
            {
                free(perHandleData);
                continue;
            }
            //关联
            //cout<<"关联SOCKET和完成端口..."<<endl;
            if(CreateIoCompletionPort((HANDLE)sockAccept,hCompletionPort,reinterpret_cast<ULONG_PTR>(perHandleData),0) == NULL)
            {
                cout<<sockAccept<<"createiocompletionport错误"<<endl;
                free(perHandleData);
                free(ioperdata);
                continue;
            }
            ////投递接收操作 必须有这句,否则 后面的数据 无法接受.
            cout<<"投递接收操作"<<endl;
            if (SOCKET_ERROR == WSARecv(sockAccept,&(ioperdata->DataBuff),1,&dwRecvBytes,&dwFlags,&(ioperdata->Overlapped),NULL))
            {
                cout << "WSARecv ERROR:" << WSAGetLastError() << endl;
            }
        }
    }
};

unsigned int WINAPI WorkerThread(LPVOID CompletionPort)//线程的执行
{
    HANDLE ComPort = (HANDLE)CompletionPort;
    DWORD BytesTransferred;
    LPPER_HANDLE_DATA PerHandleData;
    LPPER_IO_OPERATION_DATA PerIoData;
    DWORD SendBytes,RecvBytes;
    DWORD Flags = 0;
    while(TRUE)
    {
        //等待完成端口上SOCKET的完成
        //cout<<"["<<GetCurrentProcessId()<<":"<<GetCurrentThreadId()<<"]"<<":等待完成端口上SOCKET的完成"<<endl;
        GetQueuedCompletionStatus(ComPort,&BytesTransferred, reinterpret_cast<PULONG_PTR>((LPDWORD) &PerHandleData),(LPOVERLAPPED *)&PerIoData, INFINITE);
        //检查是否有错误产生
        if(BytesTransferred == 0)
        {
            //关闭SOCKET
            cout<<"["<<GetCurrentProcessId()<<":"<<GetCurrentThreadId()<<"]"<<PerHandleData->sock<<" SOCKET关闭"<<endl;
            closesocket(PerHandleData->sock);
            free(PerHandleData);
            free(PerIoData);
            continue;
        }
        //为请求服务
        cout<<"工作线程开始处理接收处理"<<endl;
        cout<<PerHandleData->sock<<" SOCKET收到消息 : "<<PerIoData->Buff<<endl;
        //回应客户端
        //ZeroMemory(PerIoData->Buff,4096);
        string option;
        for (int i = 0; i < 4096; ++i) {
            if(PerIoData->Buff[i] != ' '){
                option+=PerIoData->Buff[i];
            } else
                break;
        }
        //获取服务器端目录
        if(option == "dir"){
            WIN32_FIND_DATA fd;
            HANDLE hff = FindFirstFile(".\\*.*",&fd);;//建立一个线程
            //搜索文件
            //可以通过FindFirstFile（）函数根据当前的文件存放路径查找该文件来把待操作文件的相关属性读取到WIN32_FIND_DATA结构中去
            if (hff == INVALID_HANDLE_VALUE)//发生错误
            {
                const char *errstr = "can't list files!\n";
                printf("list file error!\n");
                if (send(PerHandleData->sock, errstr, strlen(errstr), 0) == SOCKET_ERROR)
                {
                    printf("error occurs when senging file list!\n");
                }
                //closesocket(datatcps);
                //return 0;
            }
            BOOL fMoreFiles = TRUE;
            char filerecord[4096];
            while (fMoreFiles)
            {//发送此项文件信息
                FILETIME ft;         //文件建立时间
                FileTimeToLocalFileTime(&fd.ftLastWriteTime, &ft);
                SYSTEMTIME lastwtime;     //SYSTEMTIME系统时间数据结构
                FileTimeToSystemTime(&ft, &lastwtime);
                char *dir = const_cast<char *>(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "<DIR>" : " ");
                sprintf(filerecord+strlen(filerecord), "%04d-%02d-%02d %02d:%02d  %5s %10d   %-20s\n",
                        lastwtime.wYear,
                        lastwtime.wMonth,
                        lastwtime.wDay,
                        lastwtime.wHour,
                        lastwtime.wMinute,
                        dir,
                        fd.nFileSizeLow,
                        fd.cFileName);
                fMoreFiles = FindNextFile(hff, &fd);
            }
            send(PerHandleData->sock, filerecord, strlen(filerecord), 0);
            ZeroMemory(&filerecord,4096);
            Flags = 0;
            ZeroMemory((LPVOID)&(PerIoData->Overlapped),sizeof(OVERLAPPED));
            PerIoData->DataBuff.len = 4096;
            PerIoData->DataBuff.buf = PerIoData->Buff;
            WSARecv(PerHandleData->sock,&(PerIoData->DataBuff),1,&SendBytes,0,&(PerIoData->Overlapped),NULL);
            cout << "工作线程返回OK" << std::endl;
        }
        else if (option == "get"){
            string name;
            for (int i = 4; i < 9; ++i) {
                if(PerIoData->Buff[i] != '\0'){
                    name+=PerIoData->Buff[i];
                } else
                    break;
            }
            HANDLE hFile;
            unsigned long long file_size = 0;
            char Buffer[4096];
            DWORD dwNumberOfBytesRead;
            const char * filename = name.c_str();
            hFile = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
            file_size = GetFileSize(hFile,NULL);
            cout << file_size <<endl;
            send(PerHandleData->sock,(char*)&file_size,sizeof(unsigned long long)+1,NULL);
            do
            {
                ::ReadFile(hFile,Buffer,sizeof(Buffer),&dwNumberOfBytesRead,NULL);
                ::send(PerHandleData->sock,Buffer,dwNumberOfBytesRead,0);
            } while (dwNumberOfBytesRead);
            CloseHandle(hFile);
            Flags = 0;
            ZeroMemory((LPVOID)&(PerIoData->Overlapped),sizeof(OVERLAPPED));
            PerIoData->DataBuff.len = 4096;
            PerIoData->DataBuff.buf = PerIoData->Buff;
            WSARecv(PerHandleData->sock,&(PerIoData->DataBuff),1,&SendBytes,0,&(PerIoData->Overlapped),NULL);
            cout << "工作线程返回OK" << std::endl;
        }
    }
}


#endif //ELYSIUMNET_ASYNFTPSERVER_HPP
