[CmdletBinding()]
param(
    [string]$Port = "COM6",
    [switch]$SkipBuild,
    [string]$IdfPath = "C:/Espressif/frameworks/esp-idf-v5.3.1/"
)

$ErrorActionPreference = "Stop"

function Clear-PortHolders {
    param([string]$ComPort)
    $pattern = [regex]::Escape($ComPort) + "|idf.py\\s+.*monitor|idf_monitor|pyserial|miniterm"
    $targets = Get-CimInstance Win32_Process | Where-Object {
        $_.ProcessId -ne $PID -and $_.CommandLine -match $pattern
    }

    if ($targets) {
        $targets | Select-Object ProcessId, Name, CommandLine | Format-Table -AutoSize | Out-Host
        $targets | ForEach-Object {
            Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue
        }
        Write-Host "PORT_CLEANED ($ComPort)"
    }
    else {
        Write-Host "NO_PORT_HOLDER_FOUND ($ComPort)"
    }
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..\..\..")
Set-Location $repoRoot

$env:IDF_PATH = $IdfPath
& C:\Espressif\Initialize-Idf.ps1 | Out-Null

if (-not $SkipBuild) {
    Write-Host "=== BUILD START ==="
    idf.py build
    if ($LASTEXITCODE -ne 0) { throw "Build failed with code $LASTEXITCODE" }
    Write-Host "=== BUILD OK ==="
}
else {
    Write-Host "=== BUILD SKIPPED ==="
}

Write-Host "=== CLEAN BEFORE FLASH ==="
Clear-PortHolders -ComPort $Port

Write-Host "=== FLASH START ($Port) ==="
idf.py -p $Port flash
if ($LASTEXITCODE -ne 0) { throw "Flash failed with code $LASTEXITCODE" }
Write-Host "=== FLASH OK ==="

Write-Host "=== CLEAN AFTER FLASH ==="
Clear-PortHolders -ComPort $Port

Write-Host "=== PIPELINE DONE ==="
