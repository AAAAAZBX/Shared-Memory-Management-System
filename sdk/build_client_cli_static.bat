@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo Building client_cli.exe with static linking (standalone executable)...

REM Find g++
for /f "delims=" %%i in ('where g++ 2^>nul') do (
  set "GPP=%%i"
  goto :found
)

echo [ERROR] g++ not found in PATH.
echo Please install MSYS2/MinGW-w64 and add ...\ucrt64\bin to PATH.
pause
exit /b 1

:found
for %%d in ("%GPP%") do set "GPPDIR=%%~dpd"
set "PATH=%GPPDIR%;%PATH%"

echo Using: "%GPP%"

REM Check if client SDK library exists
if not exist "lib\libsmm_client.a" (
  echo [WARNING] Client SDK library not found. Building it first...
  call build.bat
  if errorlevel 1 (
    echo Failed to build client SDK library
    pause
    exit /b 1
  )
)

REM Compile client_cli.cpp with static linking
echo.
echo Compiling client_cli.cpp with static linking...
echo This will create a standalone executable that doesn't require DLLs.
echo.
"%GPP%" -std=c++17 examples/client_cli.cpp -Iinclude -Llib -lsmm_client -o examples/client_cli_static.exe -lws2_32 -static-libgcc -static-libstdc++ -static

if errorlevel 1 (
  echo Failed to compile client_cli.cpp
  pause
  exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Output file: examples/client_cli_static.exe
echo.
echo This is a standalone executable that:
echo   - Does NOT require MinGW runtime DLLs
echo   - Does NOT require smm_client.dll
echo   - Can run on any Windows machine without installing development tools
echo.
echo Usage:
echo   examples\client_cli_static.exe [host] [port]
echo   Example: examples\client_cli_static.exe 192.168.1.100 8888
echo.
echo Note: You can copy client_cli_static.exe to any Windows machine and run it directly.
echo.
pause
