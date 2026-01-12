@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

REM 1) 找到 g++
for /f "delims=" %%i in ('where g++ 2^>nul') do (
  set "GPP=%%i"
  goto :found
)

echo [ERROR] g++ not found in PATH.
echo Please install MSYS2/MinGW-w64 and add ...\ucrt64\bin (or ...\mingw64\bin) to PATH.
pause
exit /b 1

:found
REM 2) 把 g++ 所在目录加入 PATH，确保运行时 DLL 能找到
for %%d in ("%GPP%") do set "GPPDIR=%%~dpd"
set "PATH=%GPPDIR%;%PATH%"

echo Compiling with: "%GPP%"
"%GPP%" -std=c++17 -Wall main.cpp command/commands.cpp shared_memory_pool/shared_memory_pool.cpp persistence/persistence.cpp network/protocol.cpp network/tcp_server.cpp -o main.exe -lws2_32

if errorlevel 1 (
  echo Compilation failed!
  pause
  exit /b 1
)

echo Running...
main.exe
echo main.exe exited with code %errorlevel%
pause
