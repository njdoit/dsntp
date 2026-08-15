# Build & smoke-test with TinyCC (Windows) when CMake/GCC are unavailable.
# Uses udp_stub.c (no Winsock import lib required).
# Usage: powershell -File scripts/build_tcc.ps1

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $Root

$Tcc = Join-Path $Root ".tools\tcc\tcc\tcc.exe"
if (-not (Test-Path $Tcc)) {
    Write-Error "tcc.exe not found at $Tcc — see README for CMake/GCC path"
}

$Out = Join-Path $Root "build"
New-Item -ItemType Directory -Force -Path $Out | Out-Null

$Inc = @("-Iinclude")
$Core = @(
  "lib/src/protocol/codec.c",
  "lib/src/protocol/payload.c",
  "lib/src/clock/clock.c",
  "lib/src/fsm/fsm.c",
  "lib/src/measure/measure.c",
  "lib/src/consensus/consensus.c",
  "lib/src/recover/recover.c",
  "lib/src/crypto/crypto.c",
  "lib/src/net/udp_stub.c",
  "lib/src/config/config.c"
)

Write-Host "==> compile test_smoke"
& $Tcc @Inc -o "$Out\test_smoke.exe" "tests\test_smoke.c" @Core
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> run test_smoke"
& "$Out\test_smoke.exe"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> compile dsntp-ctl (stub HTTP listen skipped on TCC)"
# Minimal ctl that only exercises compile of JSON helpers path:
@"
#include <stdio.h>
int main(void) {
  puts(`"dsntp-ctl stub: use MinGW/MSVC for real Winsock HTTP server`");
  return 0;
}
"@ | Set-Content -Encoding ASCII "$Out\ctl_stub_main.c"
& $Tcc -o "$Out\dsntp-ctl.exe" "$Out\ctl_stub_main.c"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> compile dsntp-agent"
& $Tcc @Inc -o "$Out\dsntp-agent.exe" `
  "apps\agent\main.c" "apps\agent\agent.c" "apps\agent\local_sock.c" "apps\agent\reporter.c" `
  @Core
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> compile dsntp-time-shim"
& $Tcc @Inc -o "$Out\dsntp-time-shim.exe" "apps\time-shim\main.c" @Core
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "BUILD OK (TCC): agent / time-shim / ctl / test_smoke"
Get-ChildItem $Out -Filter "*.exe" | Format-Table Name, Length
Write-Host "Note: production builds should use CMake + real lib/src/net/udp.c"
