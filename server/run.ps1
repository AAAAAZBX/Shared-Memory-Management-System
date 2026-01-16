# PowerShell wrapper script for run.bat
# This script automatically uses cmd.exe to execute the batch file

$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Path
$batFile = Join-Path $scriptPath "run.bat"

if (-not (Test-Path $batFile)) {
    Write-Host "Error: run.bat not found at $batFile" -ForegroundColor Red
    exit 1
}

Write-Host "Executing run.bat via cmd.exe..." -ForegroundColor Cyan
& cmd /c "`"$batFile`""

exit $LASTEXITCODE
