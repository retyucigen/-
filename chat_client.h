#ifndef CHAT_CLIENT_H
#define CHAT_CLIENT_H

#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

class ChatClient {
private:
    int clientSocket;
    std::string username;
    bool isConnected;
    
    std::thread receiveThread;
    
public:
    ChatClient();
    ~ChatClient();
    
    bool connectToServer(const std::string& serverIP, int port = 8080);
    void setUsername(const std::string& name);
    void start();
    void sendMessage(const std::string& message);
    void receiveMessages();
    void disconnect();
};

#endif