#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

class ClientHandler {
public:
    int clientSocket;
    std::string username;
    
    ClientHandler(int sock);
};

class ChatServer {
private:
    int serverSocket;
    std::vector<ClientHandler*> clients; //clients容器里面存放的是指针，而不是ClientHandler本身
    std::mutex clientsMutex;
    bool isRunning;
    
public:
    ChatServer();
    ~ChatServer();
    
    bool initialize(int port = 8080);
    void start();
    void stop();
    void broadcastMessage(const std::string& message, ClientHandler* sender = nullptr);
    void removeClient(ClientHandler* client);
};

#endif