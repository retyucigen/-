#include "chat_client.h"
#include <iostream>

int main() {
    ChatClient client;
    
    std::string serverIP;
    std::cout << "输入服务器IP地址 (localhost 或 127.0.0.1): ";
    std::getline(std::cin, serverIP);
    
    if (serverIP.empty()) {
        serverIP = "127.0.0.1";
    }
    
    if (!client.connectToServer(serverIP, 8080)) {
        std::cerr << "连接服务器失败" << std::endl;
        return 1;
    }
    
    std::string username;
    std::cout << "输入你的用户名: ";
    std::getline(std::cin, username);
    
    if (username.empty()) {
        username = "匿名用户";
    }
    
    client.setUsername(username);
    client.start();
    
    return 0;
}