#include "chat_server.h"
#include <iostream>
#include <signal.h>

ChatServer* g_server = nullptr;

void signalHandler(int signal) 
{
    if (signal == SIGINT && g_server) 
    {
        std::cout << "\n收到中断信号，停止服务器..." << std::endl;
        g_server->stop();
    }
}

int main() 
{
    ChatServer server;
    g_server = &server;
    
    // 注册信号处理
    signal(SIGINT, signalHandler);
    
    if (!server.initialize(8080)) 
    {
        std::cerr << "服务器初始化失败" << std::endl;
        return 1;
    }
    
    std::cout << "聊天服务器运行中... 按 Ctrl+C 停止服务器" << std::endl;
    
    server.start();
    
    std::cout << "服务器已停止" << std::endl;
    return 0;
}