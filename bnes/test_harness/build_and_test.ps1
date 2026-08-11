# H5.3 Isolation Verification Path - Standalone Build Script
# This script compiles bns_security.cc with mocked Chromium dependencies
# and runs the gtest unit tests independently of the full Chromium build chain.
#
# Usage: .\build_and_test.ps1
# Prerequisites: g++ (MSYS2/ucrt64)

$ErrorActionPreference = "Stop"

$RootDir = "S:\Ai_Agent\BNES\bnes-brave-core\bnes"
$BraveRoot = "S:\Ai_Agent\BNES\bnes-brave-core"
$HarnessDir = Join-Path $RootDir "test_harness"
$OutputDir = Join-Path $HarnessDir "out"

# Create output directory
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

$Includes = @(
    "-I`"$BraveRoot`"",
    "-I`"$HarnessDir`""
)

$Defines = @(
    "-DUNIT_TEST"
)

$Sources = @(
    "`"$RootDir\bns_security.cc`"",
    "`"$RootDir\bns_security_unittest.cc`"",
    "`"$HarnessDir\test_main.cc`""
)

$CompilerFlags = @(
    "-std=c++17",
    "-Wall",
    "-Wextra",
    "-Wno-unused-parameter"
)

$OutputExe = Join-Path $OutputDir "bns_security_tests.exe"

Write-Host "=== H5.3 Isolation Verification Path ===" -ForegroundColor Cyan
Write-Host "Compiling bns_security with mocked Chromium dependencies..."
Write-Host ""

$compileCmd = "g++ $($CompilerFlags -join ' ') $($Includes -join ' ') $($Defines -join ' ') $($Sources -join ' ') -o `"$OutputExe`" -lws2_32"
Write-Host "Command: $compileCmd" -ForegroundColor Gray
Write-Host ""

try {
    Invoke-Expression $compileCmd
    Write-Host "Compilation successful!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Running tests..." -ForegroundColor Cyan
    & $OutputExe
    $testExitCode = $LASTEXITCODE
    Write-Host ""
    if ($testExitCode -eq 0) {
        Write-Host "All tests PASSED!" -ForegroundColor Green
    } else {
        Write-Host "Tests FAILED with exit code $testExitCode" -ForegroundColor Red
    }
    exit $testExitCode
} catch {
    Write-Host "Compilation FAILED: $_" -ForegroundColor Red
    exit 1
}
