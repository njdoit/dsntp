# Start/stop 4 dsntp-agent processes on 127.0.0.1:47501-47504
# Usage:
#   powershell -ExecutionPolicy Bypass -File D:\dsntp\cproj\dsntp\scripts\lab_localhost.ps1 start
#   powershell -ExecutionPolicy Bypass -File D:\dsntp\cproj\dsntp\scripts\lab_localhost.ps1 stop
#   powershell -ExecutionPolicy Bypass -File D:\dsntp\cproj\dsntp\scripts\lab_localhost.ps1 status

param(
    [Parameter(Position = 0)]
    [ValidateSet("start", "stop", "status")]
    [string]$Action = "start"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Exe = Join-Path $Root "build-mingw\dsntp-agent.exe"
$Lab = Join-Path $Root "deploy\lab-localhost"
$LogDir = Join-Path $Root "build-mingw\lab-logs"
$PidFile = Join-Path $LogDir "pids.txt"
$NodeCount = 4

if (-not (Test-Path $Exe)) {
    throw "Missing $Exe — build first (env.ps1 + cmake --build ...)"
}

function Start-Lab {
    New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
    if (Test-Path $PidFile) {
        Write-Host "Existing pid file found; run stop first or delete $PidFile"
    }
    $pids = @()
    foreach ($id in 1..$NodeCount) {
        $conf = Join-Path $Lab "node$id.conf"
        if (-not (Test-Path $conf)) { throw "Missing conf: $conf" }
        $log = Join-Path $LogDir "node$id.log"
        if (Test-Path $log) { Remove-Item $log -Force }
        $p = Start-Process -FilePath $Exe `
            -ArgumentList @("-c", $conf) `
            -RedirectStandardError $log `
            -RedirectStandardOutput (Join-Path $LogDir "node$id.out.log") `
            -PassThru `
            -WindowStyle Hidden
        $pids += $p.Id
        Write-Host "started node$id pid=$($p.Id) port=4750$id conf=$conf"
        Start-Sleep -Milliseconds 300
    }
    $pids | Set-Content -Path $PidFile -Encoding ASCII
    Write-Host "lab: $NodeCount nodes (n=4 f=1 quorum=3). logs: $LogDir"
}

function Stop-Lab {
    if (Test-Path $PidFile) {
        Get-Content $PidFile | ForEach-Object {
            $id = $_.Trim()
            if ($id -match '^\d+$') {
                Stop-Process -Id ([int]$id) -Force -ErrorAction SilentlyContinue
                Write-Host "stopped pid=$id"
            }
        }
        Remove-Item $PidFile -Force -ErrorAction SilentlyContinue
    }
    Get-Process -Name "dsntp-agent" -ErrorAction SilentlyContinue | Stop-Process -Force
    Write-Host "lab stopped"
}

function Show-Status {
    Get-Process -Name "dsntp-agent" -ErrorAction SilentlyContinue |
        Format-Table Id, ProcessName, StartTime -AutoSize
    if (Test-Path $LogDir) {
        Write-Host "--- last lines ---"
        foreach ($id in 1..$NodeCount) {
            $log = Join-Path $LogDir "node$id.log"
            if (Test-Path $log) {
                Write-Host "=== node$id ==="
                Get-Content $log -Tail 5
            }
        }
    }
}

switch ($Action) {
    "start"  { Start-Lab }
    "stop"   { Stop-Lab }
    "status" { Show-Status }
}
