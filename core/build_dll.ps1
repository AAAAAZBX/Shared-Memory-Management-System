# PowerShell wrapper script for build_dll.bat
# This script automatically uses cmd.exe to execute the batch file

$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
$batFile = Join-Path $scriptPath "build_dll.bat"

if (-not (Test-Path $batFile)) {
    Write-Host "Error: build_dll.bat not found at $batFile" -ForegroundColor Red
    exit 1
}

Write-Host "Executing build_dll.bat via cmd.exe..." -ForegroundColor Cyan

# 切换到批处理文件所在目录
Push-Location $scriptPath

# 使用 Start-Process 直接启动 cmd.exe，避免 PowerShell 解析
$process = Start-Process -FilePath "cmd.exe" -ArgumentList "/c", "`"$batFile`"" -Wait -NoNewWindow -PassThru

Pop-Location

exit $process.ExitCode
