# Code Line Counter Tool
# Count lines in all .cpp and .h files

[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   Code Line Statistics" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$totalLines = 0
$cppFiles = @()
$hFiles = @()

# Count .cpp files
Write-Host "C++ Source Files (.cpp):" -ForegroundColor Yellow
Write-Host "----------------------------------------" -ForegroundColor Gray

Get-ChildItem -Path . -Recurse -Include *.cpp -File | ForEach-Object {
    $lines = (Get-Content $_.FullName | Measure-Object -Line).Lines
    $relativePath = $_.FullName.Replace((Get-Location).Path + "\", "")
    $cppFiles += [PSCustomObject]@{
        File = $relativePath
        Lines = $lines
    }
    Write-Host ("{0,-50} {1,6} lines" -f $relativePath, $lines) -ForegroundColor White
    $totalLines += $lines
}

$cppTotal = ($cppFiles | Measure-Object -Property Lines -Sum).Sum
Write-Host "----------------------------------------" -ForegroundColor Gray
Write-Host ("{0,-50} {1,6} lines" -f "C++ Source Files Total:", $cppTotal) -ForegroundColor Green
Write-Host ""

# Count .h files
Write-Host "C++ Header Files (.h):" -ForegroundColor Yellow
Write-Host "----------------------------------------" -ForegroundColor Gray

Get-ChildItem -Path . -Recurse -Include *.h -File | ForEach-Object {
    $lines = (Get-Content $_.FullName | Measure-Object -Line).Lines
    $relativePath = $_.FullName.Replace((Get-Location).Path + "\", "")
    $hFiles += [PSCustomObject]@{
        File = $relativePath
        Lines = $lines
    }
    Write-Host ("{0,-50} {1,6} lines" -f $relativePath, $lines) -ForegroundColor White
    $totalLines += $lines
}

$hTotal = ($hFiles | Measure-Object -Property Lines -Sum).Sum
Write-Host "----------------------------------------" -ForegroundColor Gray
Write-Host ("{0,-50} {1,6} lines" -f "C++ Header Files Total:", $hTotal) -ForegroundColor Green
Write-Host ""

# Statistics by directory
Write-Host "Statistics by Directory:" -ForegroundColor Yellow
Write-Host "----------------------------------------" -ForegroundColor Gray

$dirStats = @{}
Get-ChildItem -Path . -Recurse -Include *.cpp,*.h -File | ForEach-Object {
    $dir = $_.DirectoryName.Replace((Get-Location).Path + "\", "")
    if (-not $dirStats.ContainsKey($dir)) {
        $dirStats[$dir] = 0
    }
    $lines = (Get-Content $_.FullName | Measure-Object -Line).Lines
    $dirStats[$dir] += $lines
}

$dirStats.GetEnumerator() | Sort-Object Name | ForEach-Object {
    Write-Host ("{0,-50} {1,6} lines" -f $_.Key, $_.Value) -ForegroundColor White
}

Write-Host "----------------------------------------" -ForegroundColor Gray
Write-Host ""

# Total
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ("Total Lines: {0} lines" -f $totalLines) -ForegroundColor Green
Write-Host ("  - C++ Source Files: {0} lines" -f $cppTotal) -ForegroundColor White
Write-Host ("  - C++ Header Files: {0} lines" -f $hTotal) -ForegroundColor White
Write-Host "========================================" -ForegroundColor Cyan
