#ifndef TCPCLIENTE_H
#define TCPCLIENTE_H

#include<iostream>    //cout
#include<stdio.h> //printf
#include<string.h>    //strlen
#include<string>  //string
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include<netdb.h> //hostent

using namespace std;

/**
    TCP Client class
*/
class TcpCliente
{
private:
    int sock;
    std::string address;
    int port;
    struct sockaddr_in server;

public:
    TcpCliente();
    ~TcpCliente();
    bool conn(string, int);
    bool send_data(string data);
    string receive(int);
    int recibir(char* buffer, int len);
};


#endif // TCPCLIENTE_H
