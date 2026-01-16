@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo Building Shared Memory Management DLL...

REM 创建输出目录
if not exist "..\sdk\lib" (mkdir "..\sdk\lib")
if not exist "..\sdk\include" (mkdir "..\sdk\include")

REM 查找 g++
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

REM 定义编译选项
set "INCLUDES=-Iapi -Ishared_memory_pool -Ipersistence"
set "SOURCES=api/smm_api.cpp shared_memory_pool/shared_memory_pool.cpp persistence/persistence.cpp"
set "DLL_NAME=..\sdk\lib\smm.dll"
set "LIB_NAME=..\sdk\lib\smm.lib"
set "STATIC_LIB=..\sdk\lib\libsmm.a"

REM 编译 DLL
echo.
echo [1/3] Building DLL...
"%GPP%" -std=c++17 -shared -DSMM_BUILDING_DLL %INCLUDES% %SOURCES% -o %DLL_NAME% -Wl,--out-implib,%LIB_NAME%
if errorlevel 1 (
  echo Failed to build DLL
  pause
  exit /b 1
)
echo DLL created: %DLL_NAME%
echo Import library created: %LIB_NAME%

REM 编译静态库
echo.
echo [2/3] Building static library...
"%GPP%" -std=c++17 -c %INCLUDES% %SOURCES%
if errorlevel 1 (
  echo Failed to compile object files
  pause
  exit /b 1
)

ar rcs %STATIC_LIB% api/smm_api.o shared_memory_pool/shared_memory_pool.o persistence/persistence.o
if errorlevel 1 (
  echo Failed to create static library
  pause
  exit /b 1
)
echo Static library created: %STATIC_LIB%

REM 复制头文件到 SDK
echo.
echo [3/3] Copying header files...
copy /Y api\smm_api.h ..\sdk\include\smm_api.h >nul
echo Header file copied: ..\sdk\include\smm_api.h

REM 清理临时文件
echo.
echo Cleaning up...
del api\smm_api.o 2>nul
del shared_memory_pool\shared_memory_pool.o 2>nul
del persistence\persistence.o 2>nul

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Output files:
echo   - %DLL_NAME% (dynamic library)
echo   - %LIB_NAME% (import library)
echo   - %STATIC_LIB% (static library)
echo   - ..\sdk\include\smm_api.h (header file)
echo.
echo Usage:
echo   1. Include: #include "smm_api.h"
echo   2. Link with: -L..\sdk\lib -lsmm
echo   3. Runtime: Ensure smm.dll is in PATH
echo.
pause
