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
    const std::string COLOR_RESET = "\033[0m";
    const std::string COLOR_GREEN = "\033[32m";    // 自己消息
    const std::string COLOR_CYAN = "\033[36m";     // 系统消息
    const std::string COLOR_YELLOW = "\033[33m";   // 其他人消息
    
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