#include "chat_server.h"
#include <sstream>
#include <algorithm>

// ClientHandler 实现
ClientHandler::ClientHandler(int sock) : clientSocket(sock) {}

// ChatServer 实现
ChatServer::ChatServer() : serverSocket(-1), isRunning(false) {}

ChatServer::~ChatServer() {         
    stop();
}

bool ChatServer::initialize(int port) 
{
    // 创建socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "创建套接字失败: " << strerror(errno) << std::endl;
        return false;
    }
    
    // 设置SO_REUSEADDR选项，避免"Address already in use"错误
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "设置套接字选项失败: " << strerror(errno) << std::endl;
        close(serverSocket);
        return false;   
    }
    
    sockaddr_in serverAddr; //存储IPv4地址和端口号
    serverAddr.sin_family = AF_INET; //定义哪种地址族
    serverAddr.sin_addr.s_addr = INADDR_ANY; //设置服务器监听所有可用的网络接口，注意：serverAddr.sin_addr.s_addr      是指向uint32_t成员（4字节）
    serverAddr.sin_port = htons(port); //保存端口号
    
    //(sockaddr*)&serverAddr 将IPv4专用地址结构转换为通用地址结构，sockaddr_in用于socket定义和赋值，sockaddr用于函数参数
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "绑定端口失败: " << strerror(errno) << std::endl;
        close(serverSocket);
        return false;
    }
    
    if (listen(serverSocket, 10) == -1) {  // Linux中SOMAXCONN可能不同，使用10
        std::cerr << "监听失败: " << strerror(errno) << std::endl;
        close(serverSocket);
        return false;
    }
    
    std::cout << "服务器启动在端口 " << port << std::endl;
    return true;
}

void ChatServer::start() 
{
    isRunning = true;
    std::cout << "聊天服务器开始运行..." << std::endl;
    
    while (isRunning) 
    {
        sockaddr_in clientAddr; //创建一个空的IPv4地址结构体变量，用来存储即将连接的客户端地址信息
        socklen_t clientAddrSize = sizeof(clientAddr); //创建一个大小变量并初始化为地址结构体的大小（16字节），这个变量将告诉accept()函数我们的缓冲区有多大，并接收实际使用的大小
        
        //创建客户端socket
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        //在accept时阻塞，等待客户端connect
        
        //安全地处理accept()调用可能出现的各种错误情况
        if (clientSocket == -1) 
        {
            if (isRunning)  //正常关闭服务器(Ctrl + C) 则不进入这里（isRunning = false)
            { 
                std::cerr << "接受连接失败: " << strerror(errno) << std::endl;
            }
            continue; //返回while循环（类似于网络波动重新尝试连接）（或正常关闭服务器后退出服务器）
        }
    
        char clientIP[INET_ADDRSTRLEN]; //定义一个字符数组来存储IPv4地址字符串。INET_ADDRSTRLEN为int 16，因为IPv4地址最长为："255.255.255.255"（15字符）+ '\0'（结束符）= 16字节
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN); //将clientAddr.sin_addr中的二进制IP地址转换为点分十进制字符串，存储到clientIP数组中
        std::cout << "新的客户端连接 from " << clientIP << ":" << ntohs(clientAddr.sin_port) << std::endl;
        
        ClientHandler* client = new ClientHandler(clientSocket); //封装相关数据

        {
            std::lock_guard<std::mutex> lock(clientsMutex); //创建一个锁守卫对象，自动获取并管理互斥锁，确保当前作用域内的代码执行时，其他线程不能同时访问被保护的共享数据。
            clients.push_back(client);
        }   
        
        // 在新线程中处理客户端，注：使用lambda函数能够自动捕获局部变量，且访问私有成员更方便。
        std::thread clientThread([this, client]() 
        {
            const int BUFFER_SIZE = 1024;
            char buffer[BUFFER_SIZE];
            
            // 接收用户名，注：sizeof(buffer) - 1是为了总能使buffer不越界（为下面令最后一位为/0做铺垫）(recv() 只接收原始字节，不添加\0)
            int bytesReceived = recv(client->clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived > 0) 
            {
                buffer[bytesReceived] = '\0';
                std::string message(buffer); //将存储在C风格数组中的用户名转换为C++string
                if (message.substr(0, 8) == "USERNAME") 
                {
                    client->username = message.substr(8);//去掉用户名前的协议
                    std::string welcomeMsg = "系统: " + client->username + " 加入了聊天室";
                    std::cout << welcomeMsg << std::endl;
                    broadcastMessage(welcomeMsg);
                }
            }
            
            // 接收消息
            while (isRunning) 
            {
                //对方 → 网络 → 网卡 → 内核接收缓冲区 → 应用程序内存
                bytesReceived = recv(client->clientSocket, buffer, sizeof(buffer) - 1, 0);

                if (bytesReceived <= 0) {
                    std::cout << client->username << " 断开连接" << std::endl;
                    break;
                }
                
                buffer[bytesReceived] = '\0';
                std::string message(buffer);
                
                if (!message.empty()) //为什么检查：防止空消息被广播 
                {
                    std::string fullMessage = client->username + ": " + message;
                    std::cout << fullMessage << std::endl;
                    broadcastMessage(fullMessage, client);
                }
            }
            
            removeClient(client); 
        });
        
        clientThread.detach(); //让线程独立，且让主线程能立即继续接受新连接。线程结束时操作系统自动回收资源
    }
}

//负责将消息发送给所有在线的客户端
void ChatServer::broadcastMessage(const std::string& message, ClientHandler* sender) 
{
    //保护共享的客户端列表 clients，防止多线程同时修改导致的数据竞争。
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    for (auto client : clients) 
    {
        if (client != sender) 
        {
            //应用程序内存 → 内核发送缓冲区 → 网卡 → 网络 → 对方
            send(client->clientSocket, message.c_str(), message.length(), 0);
        }
    }
}

void ChatServer::removeClient(ClientHandler* client) 
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    //it 指向 指向那个要被关闭客户端的指针
    auto it = std::find(clients.begin(), clients.end(), client);
    if (it != clients.end()) {
        close(client->clientSocket);
        
        //为什么检查：可能客户端连接后立刻断开，还没发送用户名；可能网络异常，用户名接受失败……
        if (!client->username.empty()) {
            std::string leaveMsg = "系统: " + client->username + " 离开了聊天室";
            broadcastMessage(leaveMsg);
        }
        
        delete client; // 释放指针指向的内存，即删除实际对象，防止内存泄漏
        clients.erase(it); // 移除被释放内存的指针（无效指针）
    }
}

void ChatServer::stop() {
    isRunning = false; // 设置停止标志
    
    if (serverSocket != -1) //关闭监听socket
    {
        close(serverSocket);
        serverSocket = -1;
    }
    
    //清理所有客户端
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto client : clients) 
        {
            close(client->clientSocket);
            delete client; // 释放指针指向的内存
        }
        clients.clear(); // 移除被释放内存的指针（无效指针）
    }
}