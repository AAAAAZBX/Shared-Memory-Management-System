@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo Building Client SDK...

REM Create lib directory
if not exist "lib" (mkdir "lib")

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

REM Build static library
echo.
echo [1/4] Building static library...
"%GPP%" -std=c++17 -c src/client_sdk.cpp -Iinclude -o lib/client_sdk.o
if errorlevel 1 (
  echo Failed to compile client_sdk.cpp
  pause
  exit /b 1
)

ar rcs lib/libsmm_client.a lib/client_sdk.o
if errorlevel 1 (
  echo Failed to create static library
  pause
  exit /b 1
)
echo Static library created: lib/libsmm_client.a

REM Build DLL
echo.
echo [2/4] Building DLL...
"%GPP%" -std=c++17 -shared src/client_sdk.cpp -Iinclude -o lib/smm_client.dll -lws2_32 -Wl,--out-implib,lib/smm_client.lib
if errorlevel 1 (
  echo Failed to build DLL
  pause
  exit /b 1
)
echo DLL created: lib/smm_client.dll
echo Import library created: lib/smm_client.lib

REM Build example program
echo.
echo [3/4] Building example: client_cli...
"%GPP%" -std=c++17 examples/client_cli.cpp -Iinclude -Llib -lsmm_client -o examples/client_cli.exe -lws2_32
if errorlevel 1 (
  echo Failed to build example
  pause
  exit /b 1
)
echo Example built: examples/client_cli.exe

REM Cleanup temporary files
echo.
echo [4/4] Cleaning up...
del lib\client_sdk.o 2>nul

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Output files:
echo   - lib/libsmm_client.a (static library)
echo   - lib/smm_client.dll (dynamic library)
echo   - lib/smm_client.lib (import library)
echo   - examples/client_cli.exe (example program)
echo.
pause
