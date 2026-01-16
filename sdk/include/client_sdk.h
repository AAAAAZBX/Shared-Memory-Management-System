#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace SMMClient {

/**
 * @brief 客户端 SDK，提供类似 server 的命令行接口
 *
 * 通过 TCP 连接到 server，提供与 server 端相同的命令接口
 */
class ClientSDK {
  public:
    /**
     * @brief 构造函数
     * @param host 服务器地址（默认: "127.0.0.1"）
     * @param port 服务器端口（默认: 8888）
     */
    ClientSDK(const std::string& host = "127.0.0.1", uint16_t port = 8888);

    /**
     * @brief 析构函数
     */
    ~ClientSDK();

    /**
     * @brief 连接到服务器
     * @return 是否连接成功
     */
    bool Connect();

    /**
     * @brief 断开连接
     */
    void Disconnect();

    /**
     * @brief 检查是否已连接
     * @return 是否已连接
     */
    bool IsConnected() const;

    /**
     * @brief 执行命令（类似 server 端的 HandleCommand）
     * @param command 命令字符串（如 "alloc \"test\" \"hello\"", "read memory_00001"）
     * @return 命令执行结果（成功返回 true，失败返回 false）
     */
    bool ExecuteCommand(const std::string& command);

    /**
     * @brief 执行命令并获取输出
     * @param command 命令字符串
     * @param output 输出结果
     * @return 是否执行成功
     */
    bool ExecuteCommandWithOutput(const std::string& command, std::string& output);

    /**
     * @brief 启动交互式命令行（类似 server 的 REPL）
     * @param prompt 提示符（默认: "client> "）
     */
    void StartInteractiveCLI(const std::string& prompt = "client> ");

    /**
     * @brief 获取服务器地址
     */
    std::string GetHost() const {
        return host_;
    }

    /**
     * @brief 获取服务器端口
     */
    uint16_t GetPort() const {
        return port_;
    }

  private:
    std::string host_;
    uint16_t port_;
    void* socket_handle_; // 平台相关的 socket 句柄（Windows: SOCKET, Linux: int）
    bool connected_;

    // 内部辅助函数
    bool SendRequest(uint8_t cmd_type, const std::string& data, std::string& response);
    bool ReceiveResponse(std::string& response);
    void PrintHelp();
    void PrintLogo();
};

} // namespace SMMClient
