#include "../include/client_sdk.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

namespace SMMClient {

// 协议定义（与 server 端保持一致）
enum class CommandType : uint8_t {
    ALLOC = 0x01,
    UPDATE = 0x02,
    DELETE = 0x03,
    READ = 0x04,
    STATUS = 0x05
};

enum class ResponseCode : uint8_t {
    SUCCESS = 0x00,
    ERROR_INVALID_CMD = 0x01,
    ERROR_INVALID_PARAM = 0x02,
    ERROR_NO_MEMORY = 0x03,
    ERROR_NOT_FOUND = 0x04,
    ERROR_ALREADY_EXISTS = 0x05,
    ERROR_INTERNAL = 0xFF
};

#ifdef _WIN32
typedef SOCKET SocketHandle;
#else
typedef int SocketHandle;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

ClientSDK::ClientSDK(const std::string& host, uint16_t port)
    : host_(host), port_(port), socket_handle_(reinterpret_cast<void*>(INVALID_SOCKET)), connected_(false) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

ClientSDK::~ClientSDK() {
    Disconnect();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool ClientSDK::Connect() {
    if (connected_) {
        return true;
    }

    SocketHandle sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket\n";
        return false;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    
    if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
        // 尝试 DNS 解析
        struct hostent* host = gethostbyname(host_.c_str());
        if (host == nullptr) {
            std::cerr << "Failed to resolve host: " << host_ << "\n";
            closesocket(sock);
            return false;
        }
        memcpy(&server_addr.sin_addr, host->h_addr_list[0], host->h_length);
    }

    if (connect(sock, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to " << host_ << ":" << port_ << "\n";
        closesocket(sock);
        return false;
    }

    socket_handle_ = reinterpret_cast<void*>(sock);
    connected_ = true;
    return true;
}

void ClientSDK::Disconnect() {
    if (connected_ && socket_handle_ != reinterpret_cast<void*>(INVALID_SOCKET)) {
        closesocket(reinterpret_cast<SocketHandle>(socket_handle_));
        socket_handle_ = reinterpret_cast<void*>(INVALID_SOCKET);
        connected_ = false;
    }
}

bool ClientSDK::IsConnected() const {
    return connected_ && socket_handle_ != reinterpret_cast<void*>(INVALID_SOCKET);
}

// 解析命令并转换为协议请求
bool ClientSDK::ExecuteCommand(const std::string& command) {
    std::string output;
    return ExecuteCommandWithOutput(command, output);
}

bool ClientSDK::ExecuteCommandWithOutput(const std::string& command, std::string& output) {
    if (!IsConnected()) {
        std::cerr << "Not connected to server. Use Connect() first.\n";
        return false;
    }

    // 解析命令
    std::istringstream iss(command);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) {
        return true;  // 空命令
    }

    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    // 处理命令
    if (cmd == "help") {
        PrintHelp();
        return true;
    } else if (cmd == "alloc") {
        if (tokens.size() < 3) {
            std::cerr << "Usage: alloc \"<description>\" \"<content>\"\n";
            return false;
        }
        // 解析带引号的参数
        std::string description = tokens[1];
        std::string content = tokens[2];
        // 去掉引号
        if (description.front() == '"' && description.back() == '"') {
            description = description.substr(1, description.length() - 2);
        }
        if (content.front() == '"' && content.back() == '"') {
            content = content.substr(1, content.length() - 2);
        }
        
        std::string data = description + '\0' + content;
        std::string response;
        if (SendRequest(static_cast<uint8_t>(CommandType::ALLOC), data, response)) {
            std::cout << response << "\n";
            output = response;
            return true;
        }
    } else if (cmd == "read") {
        if (tokens.size() < 2) {
            std::cerr << "Usage: read <memory_id>\n";
            return false;
        }
        std::string response;
        if (SendRequest(static_cast<uint8_t>(CommandType::READ), tokens[1], response)) {
            std::cout << response << "\n";
            output = response;
            return true;
        }
    } else if (cmd == "update") {
        if (tokens.size() < 3) {
            std::cerr << "Usage: update <memory_id> \"<new_content>\"\n";
            return false;
        }
        std::string content = tokens[2];
        if (content.front() == '"' && content.back() == '"') {
            content = content.substr(1, content.length() - 2);
        }
        std::string data = tokens[1] + '\0' + content;
        std::string response;
        if (SendRequest(static_cast<uint8_t>(CommandType::UPDATE), data, response)) {
            std::cout << response << "\n";
            output = response;
            return true;
        }
    } else if (cmd == "free" || cmd == "delete") {
        if (tokens.size() < 2) {
            std::cerr << "Usage: " << cmd << " <memory_id>\n";
            return false;
        }
        std::string response;
        if (SendRequest(static_cast<uint8_t>(CommandType::DELETE), tokens[1], response)) {
            std::cout << response << "\n";
            output = response;
            return true;
        }
    } else if (cmd == "status") {
        std::string mode = tokens.size() > 1 ? tokens[1] : "";
        std::string response;
        if (SendRequest(static_cast<uint8_t>(CommandType::STATUS), mode, response)) {
            std::cout << response << "\n";
            output = response;
            return true;
        }
    } else if (cmd == "quit" || cmd == "exit") {
        return false;  // 退出信号
    } else {
        std::cerr << "Unknown command: " << cmd << ". Type 'help' for help.\n";
        return false;
    }

    return false;
}

void ClientSDK::StartInteractiveCLI(const std::string& prompt) {
    PrintLogo();
    
    if (!Connect()) {
        std::cerr << "Failed to connect to server at " << host_ << ":" << port_ << "\n";
        return;
    }
    
    std::cout << "Connected to server at " << host_ << ":" << port_ << "\n";
    std::cout << "Type 'help' for help, 'quit' or 'exit' to exit.\n\n";

    std::string line;
    while (true) {
        std::cout << prompt;
        std::cout.flush();
        
        if (!std::getline(std::cin, line)) {
            break;
        }
        
        // 去除首尾空白
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty()) {
            continue;
        }
        
        if (line == "quit" || line == "exit") {
            std::cout << "Bye!\n";
            break;
        }
        
        ExecuteCommand(line);
        std::cout << "\n";
    }
    
    Disconnect();
}

bool ClientSDK::SendRequest(uint8_t cmd_type, const std::string& data, std::string& response) {
    SocketHandle sock = reinterpret_cast<SocketHandle>(socket_handle_);
    
    // 构建请求包
    std::vector<uint8_t> packet;
    packet.push_back(cmd_type);
    
    uint32_t data_len = static_cast<uint32_t>(data.size());
    uint32_t net_len = htonl(data_len);
    const uint8_t* len_bytes = reinterpret_cast<const uint8_t*>(&net_len);
    packet.insert(packet.end(), len_bytes, len_bytes + 4);
    packet.insert(packet.end(), data.begin(), data.end());
    
    // 发送请求
    if (send(sock, reinterpret_cast<const char*>(packet.data()), packet.size(), 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send request\n";
        return false;
    }
    
    // 接收响应
    return ReceiveResponse(response);
}

bool ClientSDK::ReceiveResponse(std::string& response) {
    SocketHandle sock = reinterpret_cast<SocketHandle>(socket_handle_);
    
    // 接收响应头（5字节）
    uint8_t header[5];
    int received = 0;
    while (received < 5) {
        int n = recv(sock, reinterpret_cast<char*>(header + received), 5 - received, 0);
        if (n <= 0) {
            std::cerr << "Failed to receive response header\n";
            return false;
        }
        received += n;
    }
    
    ResponseCode code = static_cast<ResponseCode>(header[0]);
    uint32_t net_len;
    memcpy(&net_len, header + 1, 4);
    uint32_t data_len = ntohl(net_len);
    
    if (data_len > 1024 * 1024) {  // 最大1MB
        std::cerr << "Response too large\n";
        return false;
    }
    
    // 接收响应数据
    std::vector<char> buffer(data_len);
    received = 0;
    while (received < static_cast<int>(data_len)) {
        int n = recv(sock, buffer.data() + received, data_len - received, 0);
        if (n <= 0) {
            std::cerr << "Failed to receive response data\n";
            return false;
        }
        received += n;
    }
    
    response.assign(buffer.data(), data_len);
    
    if (code != ResponseCode::SUCCESS) {
        std::cerr << "Error [" << static_cast<int>(code) << "]: " << response << "\n";
        return false;
    }
    
    return true;
}

void ClientSDK::PrintHelp() {
    std::cout << "Available commands:\n";
    std::cout << "  alloc \"<description>\" \"<content>\"  - Allocate memory\n";
    std::cout << "  read <memory_id>                     - Read memory content\n";
    std::cout << "  update <memory_id> \"<content>\"     - Update memory content\n";
    std::cout << "  free <memory_id>                     - Free memory\n";
    std::cout << "  delete <memory_id>                   - Delete memory (alias for free)\n";
    std::cout << "  status [--memory|--block]            - Show status\n";
    std::cout << "  help                                 - Show this help\n";
    std::cout << "  quit/exit                            - Exit client\n";
}

void ClientSDK::PrintLogo() {
    std::cout << "##############################################\n";
    std::cout << "#                                            #\n";
    std::cout << "#        Shared Memory Manager Client        #\n";
    std::cout << "#                                            #\n";
    std::cout << "##############################################\n\n";
}

} // namespace SMMClient
