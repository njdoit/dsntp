# DSNTP toolchain PATH helper (CMake + w64devkit MinGW)
$Root = Split-Path -Parent $PSScriptRoot
$CMakeBin = Join-Path $Root ".tools\cmake\cmake-3.31.6-windows-x86_64\bin"
$GccExe = Get-ChildItem -Path (Join-Path $Root ".tools\w64devkit") -Recurse -Filter gcc.exe -ErrorAction SilentlyContinue |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $GccExe) {
    throw "gcc.exe not found under $($Root)\.tools\w64devkit"
}
$W64Bin = Split-Path -Parent $GccExe
$prepend = @($CMakeBin, $W64Bin) | Where-Object { $_ -and (Test-Path $_) }
$env:PATH = ($prepend -join ";") + ";" + $env:PATH
Write-Host "env.ps1: prepended"
Write-Host "  cmake: $CMakeBin"
Write-Host "  gcc:   $W64Bin"
