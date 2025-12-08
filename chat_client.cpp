#include "chat_client.h"

//这里的-1表示“未初始化的socket"
ChatClient::ChatClient() : clientSocket(-1), isConnected(false) {}

ChatClient::~ChatClient() {
    disconnect();
}

bool ChatClient::connectToServer(const std::string& serverIP, int port) 
{
    //创建socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    //错误检查
    if (clientSocket == -1) 
    {
        std::cerr << "创建套接字失败: " << strerror(errno) << std::endl;
        return false;
    }
    
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port); // 将16位的主机字节顺序转换为网络字节顺序
    
    // 转换IP地址; eg. 把"127.0.0.1"这样的字符串IP地址，转换为计算机能理解的二进制格式
    //注意：&serverAddr.sin_addr      是指向in_addr结构体（4字节）
    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0)    
    {
        std::cerr << "无效的IP地址: " << strerror(errno) << std::endl;
        close(clientSocket);
        return false;
    }
    

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) 
    {
        std::cerr << "连接服务器失败: " << strerror(errno) << std::endl;
        close(clientSocket);
        return false;
    }
    //connect函数后不进行数据交换，而是要等服务器端accept之后才进行数据交换  
    //connect同时为clientSocket中的特定数据分配本地端口和ip
    
    isConnected = true;
    std::cout << "成功连接到服务器 " << serverIP << ":" << port << std::endl;
    return true;
}

void ChatClient::setUsername(const std::string& name) 
{
    username = name;
    // 发送用户名到服务器
    std::string message = "USERNAME" + username;
    send(clientSocket, message.c_str(), message.length(), 0);
}

void ChatClient::start() {
    if (!isConnected) {
        std::cerr << "未连接到服务器" << std::endl;
        return;
    }
    
    // 启动接收消息线程
    receiveThread = std::thread(&ChatClient::receiveMessages, this);
    
    std::cout << "开始聊天 (输入 'quit' 退出)" << std::endl;
    
    std::string message;
    while (isConnected) {
        std::getline(std::cin, message);
        
        if (message == "quit") {
            break;
        }
        
        if (!message.empty()) {
            sendMessage(message);
        }
    }
    
    disconnect();
}

void ChatClient::sendMessage(const std::string& message) 
{
    if (isConnected) 
    {
        send(clientSocket, message.c_str(), message.length(), 0);
    }   
}

void ChatClient::receiveMessages() 
{
    char buffer[1024];
    int bytesReceived;
    
    while (isConnected) 
    {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesReceived <= 0) 
        {
            std::cout << "与服务器断开连接" << std::endl;
            isConnected = false;
            break;
        }
        
        buffer[bytesReceived] = '\0';
        std::cout << buffer << std::endl;
    }
}

void ChatClient::disconnect() 
{
    isConnected = false;
    
    if (clientSocket != -1) 
    {
        shutdown(clientSocket, SHUT_RDWR);
        close(clientSocket);
        clientSocket = -1;
    }
    
    if (receiveThread.joinable()) 
    {
        receiveThread.join();
    }
}