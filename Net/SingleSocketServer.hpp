//
// Created by QianL on 2020/4/10.
//

#ifndef ELYSIUMNET_SINGLESOCKETSERVER_HPP
#define ELYSIUMNET_SINGLESOCKETSERVER_HPP

#include <winsock2.h>
#include <iostream>
#include <exception>
#include <thread>
#include <string>
#include <afxres.h>
#include <windows.h>

using namespace std;

class SingleServer{
private:
    WSADATA wsadata;
    SOCKET socket_Server;
    SOCKET socket_Client;
    SOCKADDR_IN  serverAddr = {0};
    SOCKADDR_IN  clientAddr = {0};
public:
    SingleServer(){
        WORD _word = MAKEWORD(2, 2);
        int init_code = WSAStartup(_word,&this->wsadata);
        if (init_code != 0){
            cout << "Socket API��ʼ��ʧ��. �������Ϊ"+to_string(init_code) << endl;
        }
        this->socket_Server = socket(AF_INET,SOCK_STREAM,0);
        if (socket_Server == INVALID_SOCKET){
            cout << "Socket��ʼ��ʧ��" <<endl;
        }
    }
    //���ö˿ں�ֱ�Ӱ󶨼���
    void setPort(const unsigned short port){
        this->serverAddr.sin_family = AF_INET;
        this->serverAddr.sin_port = htons(port);
        this->serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
        int bind_result = bind(socket_Server,(struct sockaddr *)& serverAddr, sizeof(serverAddr));
        if (bind_result != 0){
            cout << "�󶨶˿����ַ����,������Ϊ"+to_string(bind_result) << endl;
        }
        int listen_result = listen(socket_Server,1);
        if (listen_result != 0){
            cout << "��������,������Ϊ"+to_string(listen_result) << endl;
        }
    }
    //����ip��ַ
    void setIPaddress(const char *ip_address){
        this->serverAddr.sin_addr.S_un.S_addr = inet_addr(ip_address);
    }
    //��ʼ
    void begin(){
        while(true){
            int nLen = sizeof(clientAddr);
            this->socket_Client = accept(socket_Server,(struct sockaddr*)&clientAddr, &nLen);
            cout << "�ͻ���������" << endl;
            CHAR szText[100] = {0};
            //���ջ���������
            recv(socket_Client,szText,100,0); //���պ�����һֱ��������ģʽ���ȴ��������˷������ݵĵ�����
            string s = szText;
            if(s == "END"){
                break;
            }
            printf("%s\n",szText);
            CHAR szSend[100] = "Hello Client";
            send(socket_Client,szSend,sizeof(szSend),0);
        }
    }
    //�ر�����
    void close(){
        closesocket(socket_Client);
        closesocket(socket_Server);
    }
    ~SingleServer(){
        closesocket(socket_Client);
        closesocket(socket_Server);
        WSACleanup();
    }

};

#endif //ELYSIUMNET_SINGLESOCKETSERVER_HPP
