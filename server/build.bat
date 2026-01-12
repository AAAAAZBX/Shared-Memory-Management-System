@echo off
cd /d %~dp0
g++ main.cpp command/commands.cpp shared_memory_pool/shared_memory_pool.cpp persistence/persistence.cpp -o main.exe -lws2_32
if %errorlevel% equ 0 (
    echo Compilation successful!
) else (
    echo Compilation failed!
    pause
)





