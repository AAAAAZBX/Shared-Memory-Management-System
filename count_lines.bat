@echo off
chcp 65001 >nul
echo ========================================
echo    代码文件行数统计
echo ========================================
echo.

REM 使用 PowerShell 执行统计脚本
powershell.exe -ExecutionPolicy Bypass -File "%~dp0count_lines.ps1"

pause
