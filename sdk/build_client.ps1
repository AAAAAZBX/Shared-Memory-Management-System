# PowerShell script to build client.cpp

$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
Push-Location $scriptPath

Write-Host "Building client.cpp..." -ForegroundColor Green

# Find g++
$gpp = Get-Command g++ -ErrorAction SilentlyContinue
if (-not $gpp) {
    Write-Host "[ERROR] g++ not found in PATH." -ForegroundColor Red
    Write-Host "Please install MSYS2/MinGW-w64 and add ...\ucrt64\bin to PATH."
    Read-Host "Press Enter to exit"
    Pop-Location
    exit 1
}

$gppPath = $gpp.Source
$gppDir = Split-Path -Parent $gppPath
$env:PATH = "$gppDir;$env:PATH"

Write-Host "Using: $gppPath"

# Check if client SDK library exists
if (-not (Test-Path "lib\libsmm_client.a")) {
    Write-Host "[WARNING] Client SDK library not found. Building it first..." -ForegroundColor Yellow
    & cmd /c "build.bat"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Failed to build client SDK library" -ForegroundColor Red
        Read-Host "Press Enter to exit"
        Pop-Location
        exit 1
    }
}

# Compile client.cpp
Write-Host ""
Write-Host "Compiling client.cpp..." -ForegroundColor Yellow
$compileArgs = @(
    "-std=c++17",
    "include/client.cpp",
    "-Iinclude",
    "-Llib",
    "-lsmm_client",
    "-o", "client.exe",
    "-lws2_32"
)
& $gppPath $compileArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to compile client.cpp" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    Pop-Location
    exit 1
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Output file: client.exe"
Write-Host ""
Write-Host "Usage:"
Write-Host "  client.exe [host] [port]"
Write-Host "  Example: client.exe 127.0.0.1 8888"
Write-Host ""
Pop-Location
