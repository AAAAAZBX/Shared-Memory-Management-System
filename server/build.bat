@echo off
cd /d %~dp0
g++ main.cpp commands.cpp shared_memory_pool.cpp -o main.exe
if %errorlevel% equ 0 (
    echo Compilation successful!
) else (
    echo Compilation failed!
    pause
)





