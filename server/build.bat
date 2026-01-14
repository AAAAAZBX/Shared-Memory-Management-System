@echo off
cd /d %~dp0
g++ main.cpp command/commands.cpp ../core/shared_memory_pool/shared_memory_pool.cpp ../core/persistence/persistence.cpp network/protocol.cpp network/tcp_server.cpp -o main.exe -lws2_32
if %errorlevel% equ 0 (
    echo Compilation successful!
) else (
    echo Compilation failed!
    pause
)





