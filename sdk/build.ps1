# PowerShell wrapper script for build.bat
# This script automatically uses cmd.exe to execute the batch file

$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
$batFile = Join-Path $scriptPath "build.bat"

if (-not (Test-Path $batFile)) {
    Write-Host "Error: build.bat not found at $batFile" -ForegroundColor Red
    exit 1
}

Write-Host "Executing build.bat via cmd.exe..." -ForegroundColor Cyan
& cmd /c "`"$batFile`""

exit $LASTEXITCODE
