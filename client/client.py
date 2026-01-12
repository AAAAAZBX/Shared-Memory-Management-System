#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Shared Memory Management System - TCP Client
客户端程序，用于连接服务器并操作共享内存池
"""

import socket
import struct
from enum import IntEnum
from typing import Optional, Tuple


class CommandType(IntEnum):
    """命令类型"""
    ALLOC = 0x01   # 分配内存
    UPDATE = 0x02  # 更新内容
    DELETE = 0x03  # 删除/释放内存
    READ = 0x04    # 读取内容
    STATUS = 0x05  # 查询状态
    PING = 0x06    # 心跳检测


class ResponseCode(IntEnum):
    """响应状态码"""
    SUCCESS = 0x00              # 成功
    ERROR_INVALID_CMD = 0x01     # 无效命令
    ERROR_INVALID_PARAM = 0x02   # 无效参数
    ERROR_NO_MEMORY = 0x03       # 内存不足
    ERROR_NOT_FOUND = 0x04       # 不存在
    ERROR_ALREADY_EXISTS = 0x05  # 已存在
    ERROR_INTERNAL = 0xFF        # 内部错误


class MemoryClient:
    """共享内存管理客户端"""
    
    def __init__(self, host: str = "127.0.0.1", port: int = 8888):
        """
        初始化客户端
        
        Args:
            host: 服务器地址
            port: 服务器端口（默认8888）
        """
        self.host = host
        self.port = port
        self.sock: Optional[socket.socket] = None
    
    def connect(self) -> bool:
        """
        连接到服务器
        
        Returns:
            bool: 连接是否成功
        """
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(10)  # 设置超时时间10秒
            self.sock.connect((self.host, self.port))
            print(f"Connected to server {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"Failed to connect: {e}")
            if self.sock:
                try:
                    self.sock.close()
                except:
                    pass
                self.sock = None
            return False
    
    def disconnect(self):
        """断开连接"""
        if self.sock:
            try:
                self.sock.close()
                print("Disconnected from server")
            except:
                pass
            finally:
                self.sock = None
    
    def _send_request(self, cmd: CommandType, data: bytes = b"") -> Tuple[ResponseCode, bytes]:
        """
        发送请求并接收响应
        
        Args:
            cmd: 命令类型
            data: 数据内容（字节）
        
        Returns:
            Tuple[ResponseCode, bytes]: (状态码, 响应数据)
        """
        if not self.sock:
            raise RuntimeError("Not connected to server")
        
        # 构建请求包：[命令类型: 1字节] [数据长度: 4字节(大端序)] [数据内容: N字节]
        cmd_byte = cmd.value.to_bytes(1, 'big')
        data_len = len(data).to_bytes(4, 'big')
        request = cmd_byte + data_len + data
        
        try:
            # 发送请求
            self.sock.sendall(request)
            
            # 接收响应头（5字节）
            header = self.sock.recv(5)
            if len(header) < 5:
                raise RuntimeError("Incomplete response header")
            
            # 解析响应头：[状态码: 1字节] [数据长度: 4字节(大端序)]
            code = ResponseCode(header[0])
            data_len = struct.unpack('>I', header[1:5])[0]  # 大端序无符号整数
            
            # 接收响应数据
            response_data = b""
            while len(response_data) < data_len:
                chunk = self.sock.recv(data_len - len(response_data))
                if not chunk:
                    raise RuntimeError("Incomplete response data")
                response_data += chunk
            
            return code, response_data
            
        except Exception as e:
            raise RuntimeError(f"Request failed: {e}")
    
    def alloc(self, description: str, content: str) -> Optional[str]:
        """
        分配内存
        
        Args:
            description: 描述信息
            content: 内容
        
        Returns:
            str: Memory ID（成功时），None（失败时）
        """
        # 数据格式：description\0content
        data = (description + '\0' + content).encode('utf-8')
        code, response = self._send_request(CommandType.ALLOC, data)
        
        if code == ResponseCode.SUCCESS:
            memory_id = response.decode('utf-8')
            print(f"Memory allocated: {memory_id}")
            return memory_id
        else:
            error_msg = response.decode('utf-8', errors='ignore')
            print(f"Allocation failed [{code.name}]: {error_msg}")
            return None
    
    def update(self, memory_id: str, new_content: str) -> bool:
        """
        更新内存内容
        
        Args:
            memory_id: Memory ID
            new_content: 新内容
        
        Returns:
            bool: 是否成功
        """
        # 数据格式：memory_id\0new_content
        data = (memory_id + '\0' + new_content).encode('utf-8')
        code, response = self._send_request(CommandType.UPDATE, data)
        
        if code == ResponseCode.SUCCESS:
            result = response.decode('utf-8', errors='ignore')
            print(f"Update successful: {result}")
            return True
        else:
            error_msg = response.decode('utf-8', errors='ignore')
            print(f"Update failed [{code.name}]: {error_msg}")
            return False
    
    def delete(self, memory_id: str) -> bool:
        """
        删除/释放内存
        
        Args:
            memory_id: Memory ID
        
        Returns:
            bool: 是否成功
        """
        data = memory_id.encode('utf-8')
        code, response = self._send_request(CommandType.DELETE, data)
        
        if code == ResponseCode.SUCCESS:
            result = response.decode('utf-8', errors='ignore')
            print(f"Delete successful: {result}")
            return True
        else:
            error_msg = response.decode('utf-8', errors='ignore')
            print(f"Delete failed [{code.name}]: {error_msg}")
            return False
    
    def read(self, memory_id: str) -> Optional[str]:
        """
        读取内存内容
        
        Args:
            memory_id: Memory ID
        
        Returns:
            str: 内存内容（成功时），None（失败时）
        """
        data = memory_id.encode('utf-8')
        code, response = self._send_request(CommandType.READ, data)
        
        if code == ResponseCode.SUCCESS:
            content = response.decode('utf-8', errors='ignore')
            print(f"Read successful:\n{content}")
            return content
        else:
            error_msg = response.decode('utf-8', errors='ignore')
            print(f"Read failed [{code.name}]: {error_msg}")
            return None
    
    def status(self) -> Optional[str]:
        """
        查询所有内存块状态
        
        Returns:
            str: 状态信息（成功时），None（失败时）
        """
        code, response = self._send_request(CommandType.STATUS)
        
        if code == ResponseCode.SUCCESS:
            status_info = response.decode('utf-8', errors='ignore')
            print(f"Status:\n{status_info}")
            return status_info
        else:
            error_msg = response.decode('utf-8', errors='ignore')
            print(f"Status query failed [{code.name}]: {error_msg}")
            return None
    
    def ping(self) -> bool:
        """
        心跳检测
        
        Returns:
            bool: 是否成功
        """
        code, response = self._send_request(CommandType.PING)
        
        if code == ResponseCode.SUCCESS:
            result = response.decode('utf-8', errors='ignore')
            print(f"Ping successful: {result}")
            return True
        else:
            error_msg = response.decode('utf-8', errors='ignore')
            print(f"Ping failed [{code.name}]: {error_msg}")
            return False
    
    def __enter__(self):
        """上下文管理器入口"""
        if not self.connect():
            raise RuntimeError(f"Failed to connect to server {self.host}:{self.port}")
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """上下文管理器出口"""
        self.disconnect()


def main():
    """示例用法"""
    # 使用上下文管理器（推荐）
    # 注意：如果服务器未运行，会抛出异常
    try:
        with MemoryClient("172.29.57.127", 8888) as client:
            # 心跳检测
            print("=" * 50)
            print("Testing PING...")
            client.ping()
            
            # 分配内存
            print("\n" + "=" * 50)
            print("Testing ALLOC...")
            memory_id = client.alloc("Local Client", "Hello, World!")
            
            if memory_id:
                # 读取内存
                print("\n" + "=" * 50)
                print("Testing READ...")
                client.read(memory_id)
                
                # 更新内存
                print("\n" + "=" * 50)
                print("Testing UPDATE...")
                client.update(memory_id, "Updated content!")
                
                # 再次读取
                print("\n" + "=" * 50)
                print("Testing READ after UPDATE...")
                client.read(memory_id)
                
                # 查询状态
                print("\n" + "=" * 50)
                print("Testing STATUS...")
                client.status()
                
                # 删除内存（可选）
                # print("\n" + "=" * 50)
                # print("Testing DELETE...")
                # client.delete(memory_id)
    except RuntimeError as e:
        print(f"\nError: {e}")
        print("\nPlease make sure the server is running:")
        print("  1. Start the server: cd server && .\\run.bat")
        print("  2. Check the server IP and port")
        print("  3. Verify firewall settings")
        return
    
    # 或者手动管理连接
    # client = MemoryClient("127.0.0.1", 8888)
    # if client.connect():
    #     try:
    #         client.ping()
    #         memory_id = client.alloc("My Data", "Test content")
    #         if memory_id:
    #             client.read(memory_id)
    #     finally:
    #         client.disconnect()


if __name__ == "__main__":
    main()
