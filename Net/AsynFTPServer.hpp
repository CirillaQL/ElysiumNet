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
}PER_IO_OPERATION_DATA,* LPPER_IO_OPERATION_DATA;

unsigned int WINAPI WorkerThread(LPVOID CompletionPort);//�����߳�

class FTPServer{
private:
    WSADATA wsadata;
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
            cout << "��ʼ��ʧ��" << endl;
            exit(1);
        }
        /*
         * ��ɶ˿ڵĴ���
         * */
        hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr,0,0);
        /*
         * ���ϵͳ��Ϣ�еĺ���������������Ϊ8���ĺ˰��߳�
         * */
        auto a = new SystemInfo();
        for (int i = 1; i <= a->get_Number_Processors() * 2; i++) {
            cout<<"�����������߳�"<<i<<endl;
            _beginthreadex(nullptr, 0, WorkerThread, (LPVOID)hCompletionPort, 0, nullptr);
            Sleep(300);
        }


        sockListen = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);//���Ƿ������׽��֣������ص�IO�׽��֡�

        if(setsockopt(sockListen,SOL_SOCKET,SO_REUSEADDR,(const char *)&nReuseAddr,sizeof(int)) != 0)
        {
            cout<<"setsockopt����"<<endl;
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
        cout<<"Socket Listen ������..."<<endl;
    }

    void start(){
        while(true){
            //��ʱ���յ�һ�����ӣ�
            sockAccept = WSAAccept(sockListen,nullptr,nullptr,nullptr,0);

            /*
             *
             * perHandleData ��  LPPER_HANDLE_DATA
             *
             * typedef struct _PER_HANDLE_DATA
             * {
             *     SOCKET sock;
             * }PER_HANDLE_DATA,* LPPER_HANDLE_DATA;
             *
             *
             * */
            perHandleData = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
            if(perHandleData == nullptr)
                continue;
            cout<<"Accept:socket number "<<sockAccept<<"����"<<endl;
            perHandleData->sock = sockAccept;

            ioperdata = (LPPER_IO_OPERATION_DATA)malloc(sizeof(PER_IO_OPERATION_DATA));

            memset(&(ioperdata->Overlapped),0,sizeof(OVERLAPPED));

            ioperdata->DataBuff.len = BUF_SIZE;
            ioperdata->DataBuff.buf = ioperdata->Buff;

            //cout << "IOPERDATA->DataBuff.buf: " << ioperdata->DataBuff.buf <<endl;
            if( ioperdata == nullptr)
            {
                free(perHandleData);
                cout << "��ȡ��ϢΪ��";
                continue;
            }
            //����
            cout<<"����SOCKET����ɶ˿�..."<<endl;
            if(CreateIoCompletionPort((HANDLE)sockAccept,hCompletionPort,reinterpret_cast<ULONG_PTR>(perHandleData),0) == nullptr)
            {
                cout<<sockAccept<<"createiocompletionport����"<<endl;
                free(perHandleData);
                free(ioperdata);
                continue;
            }
            //WSARecvΪͶ�ݲ���
            cout<<"�յ��ͻ������ӣ�Ͷ�ݵ���ɶ˿ڽ��������߳�"<<endl;
            if (SOCKET_ERROR == WSARecv(sockAccept, &(ioperdata->DataBuff), 1, &dwRecvBytes, &dwFlags, &(ioperdata->Overlapped), nullptr))
            {
                cout << "WSARecv ERROR:" << WSAGetLastError() << endl;
            }
        }
    }
};


/*
 * �����߳�
 */
unsigned int WINAPI WorkerThread(LPVOID CompletionPort)//�̵߳�ִ��
{
    auto ComPort = (HANDLE)CompletionPort;
    DWORD BytesTransferred;
    LPPER_HANDLE_DATA PerHandleData;
    LPPER_IO_OPERATION_DATA PerIoData;
    DWORD SendBytes,RecvBytes;
    while(TRUE) {
        //�ȴ���ɶ˿���SOCKET�����
        cout<<"��������: "<< GetCurrentProcessId() <<"  �����߳�: "<<GetCurrentThreadId()<<" "<<":�ȴ��ͻ�������"<<endl;
        GetQueuedCompletionStatus(ComPort, &BytesTransferred, reinterpret_cast<PULONG_PTR>((LPDWORD) &PerHandleData),(LPOVERLAPPED *) &PerIoData, INFINITE);
        //����Ƿ��д������
        if (BytesTransferred == 0) {
            //�ر�SOCKET
            cout << "[" << GetCurrentProcessId() << ":" << GetCurrentThreadId() << "]" << PerHandleData->sock << " SOCKET�ر�" << endl;
            closesocket(PerHandleData->sock);
            free(PerHandleData);
            free(PerIoData);
            continue;
        }
        //Ϊ�������
        cout << "�����߳�: " << GetCurrentThreadId() << "��ʼ������մ���" << endl;
        cout << "�����߳�: "<< GetCurrentThreadId() << " �յ���Ϣ : " << PerIoData->Buff << " �����Ϣ�Ĵ�С: "<< sizeof(PerIoData->Buff) << endl;
        //��Ӧ�ͻ���
        //ZeroMemory(PerIoData->Buff,4096);
        string option;
        for (int i = 0; i < 8192; ++i) {
            if (PerIoData->Buff[i] != ' ') {
                option += PerIoData->Buff[i];
            } else
                break;
        }

        //
        //cout << option << endl;
        //��ȡ��������Ŀ¼
        if (option == "dir") {
            WIN32_FIND_DATA fd;
            HANDLE hff = FindFirstFile(".\\*.*", &fd);;//����һ���߳�
            //�����ļ�
            //����ͨ��FindFirstFile�����������ݵ�ǰ���ļ����·�����Ҹ��ļ����Ѵ������ļ���������Զ�ȡ��WIN32_FIND_DATA�ṹ��ȥ
            if (hff == INVALID_HANDLE_VALUE)//��������
            {
                const char *errstr = "can't list files!\n";
                printf("list file error!\n");
                if (send(PerHandleData->sock, errstr, strlen(errstr), 0) == SOCKET_ERROR) {
                    printf("error occurs when senging file list!\n");
                }
                //closesocket(datatcps);
                //return 0;
            }
            BOOL fMoreFiles = TRUE;
            char filerecord[4096];
            while (fMoreFiles) {//���ʹ����ļ���Ϣ
                FILETIME ft;         //�ļ�����ʱ��
                FileTimeToLocalFileTime(&fd.ftLastWriteTime, &ft);
                SYSTEMTIME lastwtime;     //SYSTEMTIMEϵͳʱ�����ݽṹ
                FileTimeToSystemTime(&ft, &lastwtime);
                char *dir = const_cast<char *>(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "<DIR>" : " ");
                sprintf(filerecord + strlen(filerecord), "%04d-%02d-%02d %02d:%02d  %5s %10d   %-20s\n",
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
            cout << filerecord << endl;
            send(PerHandleData->sock, filerecord, strlen(filerecord), 0);
            ZeroMemory(&filerecord, 4096);
            ZeroMemory((LPVOID) &(PerIoData->Overlapped), sizeof(OVERLAPPED));
            PerIoData->DataBuff.len = 4096;
            PerIoData->DataBuff.buf = PerIoData->Buff;
            WSARecv(PerHandleData->sock, &(PerIoData->DataBuff), 1, &SendBytes, 0, &(PerIoData->Overlapped), nullptr);
            cout << "�����̷߳���OK" << std::endl;
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
            hFile = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
            if (hFile == INVALID_HANDLE_VALUE) {
                cout << "�ļ�����û�и��ļ�" << endl;
                return 0;
            }
            file_size = GetFileSize(hFile,nullptr);
            cout << file_size <<endl;
            send(PerHandleData->sock, (char*)&file_size,sizeof(unsigned long long)+1, 0);
            do
            {
                ::ReadFile(hFile,Buffer,sizeof(Buffer),&dwNumberOfBytesRead,nullptr);
                ::send(PerHandleData->sock,Buffer,dwNumberOfBytesRead,0);
            } while (dwNumberOfBytesRead);
            CloseHandle(hFile);
            ZeroMemory((LPVOID)&(PerIoData->Overlapped),sizeof(OVERLAPPED));
            PerIoData->DataBuff.len = 4096;
            PerIoData->DataBuff.buf = PerIoData->Buff;
            WSARecv(PerHandleData->sock,&(PerIoData->DataBuff),1,&SendBytes,0,&(PerIoData->Overlapped),nullptr);
            cout << "�����̷߳���OK" << std::endl;
        }
        else if (option == "post"){
            string name;
            for (int i = 5; i < 8192; ++i) {
                if(PerIoData->Buff[i] != '\0'){
                    name+=PerIoData->Buff[i];
                } else
                    break;
            }
            cout << "ִ��post" << endl;
            cout << "׼����ȡ�ļ�: "<< name << "�ļ���" << endl;

            /*
             * ����һ���µ�Socket
             */

            SOCKET s_server = socket(AF_INET, SOCK_STREAM, 0);

            SOCKADDR_IN server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
            server_addr.sin_port = htons(9091);

            if (bind(s_server, (SOCKADDR *)&server_addr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
                cout << "�׽��ְ�ʧ�ܣ�" << endl;
                WSACleanup();
            }
            else {
                cout << "�׽��ְ󶨳ɹ���" << endl;
            }
            //�����׽���Ϊ����״̬
            if (listen(s_server, SOMAXCONN) < 0) {
                cout << "���ü���״̬ʧ�ܣ�" << endl;
                WSACleanup();
            }
            else {
                cout << "���ü���״̬�ɹ���" << endl;
            }
            cout << "��������ڼ������ӣ����Ժ�...." << endl;

            auto len = sizeof(SOCKADDR);
            auto s_accept = accept(s_server, (SOCKADDR *)&server_addr, reinterpret_cast<int *>(&len));
            if (s_accept == SOCKET_ERROR) {
                cout << "����ʧ�ܣ�" << endl;
                WSACleanup();
                return 0;
            }
            cout << "���ӽ�����׼����������" << endl;

            char  buff[8192];
            FILE *fp;
            int  n;

            if((fp = fopen(name.c_str(),"ab") ) == NULL )
            {
                printf("File.\n");
                closesocket(s_accept);
                exit(1);
            }

            while(1){
                n = recv(s_accept, buff, BUF_SIZE,0);
                if(n == 0)
                    break;
                fwrite(buff, 1, n, fp);
            }
            buff[n] = '\0';
            printf("recv msg from client: %s\n", buff);

            fclose(fp);

            closesocket(s_accept);



            ZeroMemory((LPVOID)&(PerIoData->Overlapped),sizeof(OVERLAPPED));
            PerIoData->DataBuff.len = 4096;
            PerIoData->DataBuff.buf = PerIoData->Buff;
            WSARecv(PerHandleData->sock,&(PerIoData->DataBuff),1,&SendBytes,0,&(PerIoData->Overlapped),nullptr);
            cout << "�����̷߳���OK" << std::endl;
        }
    }
}
#endif //ELYSIUMNET_ASYNFTPSERVER_HPP
