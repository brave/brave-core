#Requires -Version 5.1
<#
.SYNOPSIS
  H5.2 environment inventory for BnesBrowser / brave-core + Chromium GN.

.DESCRIPTION
  Non-empty preflight: records toolchain, disk, layout, and bnes GN wiring.
  Does NOT download Chromium. Does NOT claim H5.2 complete without gn gen.

  Exit codes:
    0  — environment ready for gn gen / compile path (all critical gates PASS)
    2  — inventory ran; one or more critical gates FAIL (expected until checkout)
    1  — script error

.PARAMETER ChromiumSrc
  Optional path to chromium src (default: auto-detect common layouts).

.PARAMETER MinFreeGb
  Minimum free space recommended on the drive hosting chromium (default 80).
#>
[CmdletBinding()]
param(
  [string]$ChromiumSrc = "",
  [int]$MinFreeGb = 80
)

$ErrorActionPreference = "Continue"
$BraveCore = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$BnesDir = Join-Path $BraveCore "bnes"
$WorkspaceRoot = (Resolve-Path (Join-Path $BraveCore "..")).Path

function Test-Cmd($Name) {
  $c = Get-Command $Name -ErrorAction SilentlyContinue
  if ($c) { return $c.Source }
  return $null
}

function Get-DriveFreeGb($Path) {
  try {
    $root = [System.IO.Path]::GetPathRoot((Resolve-Path $Path -ErrorAction Stop).Path)
    $driveLetter = $root.TrimEnd('\').TrimEnd(':')
    $d = Get-PSDrive -Name $driveLetter -ErrorAction Stop
    return [math]::Round($d.Free / 1GB, 2)
  } catch {
    return $null
  }
}

Write-Host "=== H5.2 BnesBrowser / Chromium environment check ==="
Write-Host "timestamp_utc $(Get-Date -Format o)"
Write-Host "brave_core $BraveCore"
Write-Host "workspace  $WorkspaceRoot"
Write-Host "os $([System.Environment]::OSVersion.VersionString)"
Write-Host "arch $([System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture)"

$results = @()
function Add-Result($Id, $Status, $Detail) {
  $script:results += [pscustomobject]@{ id = $Id; status = $Status; detail = $Detail }
  $tag = if ($Status -eq "PASS") { "PASS" } elseif ($Status -eq "WARN") { "WARN" } else { "FAIL" }
  Write-Host ("[{0}] {1}: {2}" -f $tag, $Id, $Detail)
}

# --- Host tools ---
$node = Test-Cmd "node"
if ($node) {
  $nv = (& node -v 2>$null)
  Add-Result "node" "PASS" "$nv @ $node"
  # brave-core package.json devEngines: >=24.16 <25
  if ($nv -match 'v(\d+)\.(\d+)') {
    $major = [int]$Matches[1]
    $minor = [int]$Matches[2]
    if ($major -lt 24 -or ($major -eq 24 -and $minor -lt 16) -or $major -ge 25) {
      Add-Result "node_version_brave" "WARN" "brave-core expects Node >=24.16 <25; got $nv"
    } else {
      Add-Result "node_version_brave" "PASS" "within brave-core range ($nv)"
    }
  }
} else {
  Add-Result "node" "FAIL" "node not on PATH"
}

$pnpm = Test-Cmd "pnpm"
if ($pnpm) {
  $pv = ""
  try { $pv = (& pnpm.cmd -v 2>$null | Select-Object -First 1) } catch { }
  if (-not $pv) { try { $pv = (pnpm --version 2>$null | Select-Object -First 1) } catch { } }
  if (-not $pv) { $pv = "present" }
  Add-Result "pnpm" "PASS" "$pv @ $pnpm"
} else {
  Add-Result "pnpm" "FAIL" "pnpm not on PATH (brave-core requires pnpm >=11.9)"
}

$git = Test-Cmd "git"
Add-Result "git" $(if ($git) { "PASS" } else { "FAIL" }) $(if ($git) { $git } else { "git missing" })

$python = Test-Cmd "python"
if (-not $python) { $python = Test-Cmd "python3" }
Add-Result "python" $(if ($python) { "PASS" } else { "WARN" }) $(if ($python) { $python } else { "python missing (needed for depot_tools/gclient)" })

# --- Chromium toolchain (critical for H5.2 complete) ---
$gn = Test-Cmd "gn"
$ninja = Test-Cmd "ninja"
$gclient = Test-Cmd "gclient"
Add-Result "gn" $(if ($gn) { "PASS" } else { "FAIL" }) $(if ($gn) { $gn } else { "gn not on PATH (comes with depot_tools after chromium sync)" })
Add-Result "ninja" $(if ($ninja) { "PASS" } else { "FAIL" }) $(if ($ninja) { $ninja } else { "ninja not on PATH" })
Add-Result "gclient" $(if ($gclient) { "PASS" } else { "FAIL" }) $(if ($gclient) { $gclient } else { "gclient not on PATH (install depot_tools)" })

$depotVendor = Join-Path $BraveCore "vendor\depot_tools"
Add-Result "depot_tools_vendor" $(if (Test-Path $depotVendor) { "PASS" } else { "FAIL" }) $(
  if (Test-Path $depotVendor) { $depotVendor } else { "missing $depotVendor (created by pnpm run init / sync)" }
)

# --- Visual Studio (Windows) ---
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
  $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
  if ($vsPath) {
    Add-Result "vs_msvc" "PASS" $vsPath
  } else {
    Add-Result "vs_msvc" "FAIL" "vswhere found but no VC Tools x86/x64 workload"
  }
} else {
  Add-Result "vs_msvc" "FAIL" "vswhere.exe missing — install VS Build Tools with C++ desktop workload"
}

# --- Layout / chromium src ---
$candidates = @()
if ($ChromiumSrc) { $candidates += $ChromiumSrc }
$candidates += @(
  (Join-Path $BraveCore "..\src"),
  (Join-Path $BraveCore "src"),
  (Join-Path $WorkspaceRoot "src"),
  "C:\src\chromium\src",
  "C:\bnes-browser\src"
)

$foundSrc = $null
foreach ($c in $candidates) {
  $full = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($c)
  if (Test-Path (Join-Path $full "BUILD.gn")) {
    $foundSrc = $full
    break
  }
}

if ($foundSrc) {
  Add-Result "chromium_src" "PASS" $foundSrc
} else {
  Add-Result "chromium_src" "FAIL" "no chromium src with BUILD.gn (need full gclient checkout; not just brave-core)"
}

# Expected brave mount: <chromium_src>/brave -> this tree when integrated
$braveMount = $null
if ($foundSrc) {
  $braveMount = Join-Path $foundSrc "brave"
  if (Test-Path $braveMount) {
    Add-Result "brave_mount" "PASS" $braveMount
  } else {
    Add-Result "brave_mount" "FAIL" "chromium src exists but src/brave missing (symlink or clone bnes-brave-core here)"
  }
} else {
  Add-Result "brave_mount" "FAIL" "blocked until chromium_src exists"
}

# --- Disk ---
$driveProbe = if ($foundSrc) { $foundSrc } else { "C:\" }
$freeC = Get-DriveFreeGb "C:\"
$freeS = Get-DriveFreeGb "S:\"
$freeProbe = Get-DriveFreeGb $driveProbe
Write-Host "disk_free_C_GB $freeC"
Write-Host "disk_free_S_GB $freeS"
if ($null -ne $freeProbe -and $freeProbe -ge $MinFreeGb) {
  Add-Result "disk_for_chromium" "PASS" ("{0} free {1} GB (min {2})" -f $driveProbe, $freeProbe, $MinFreeGb)
} elseif ($null -ne $freeC -and $freeC -ge $MinFreeGb) {
  Add-Result "disk_for_chromium" "WARN" ("probe drive low; C: has {0} GB free — put checkout on C: (min {1})" -f $freeC, $MinFreeGb)
} else {
  Add-Result "disk_for_chromium" "FAIL" ("need ~{0}+ GB free for chromium; C={1} S={2}" -f $MinFreeGb, $freeC, $freeS)
}

# S: workspace free space note
if ($null -ne $freeS -and $freeS -lt 40) {
  Add-Result "workspace_drive_S" "WARN" ("S: only {0} GB free — do NOT put full chromium on S:" -f $freeS)
} else {
  Add-Result "workspace_drive_S" "PASS" ("S: free {0} GB" -f $freeS)
}

# --- bnes source integrity (always checkable) ---
$required = @(
  "BUILD.gn",
  "bns_security.cc",
  "bns_security.h",
  "bns_security_unittest.cc",
  "bns_constants.h"
)
$missing = @()
foreach ($f in $required) {
  if (-not (Test-Path (Join-Path $BnesDir $f))) { $missing += $f }
}
if ($missing.Count -eq 0) {
  Add-Result "bnes_sources" "PASS" "all required files under bnes/"
} else {
  Add-Result "bnes_sources" "FAIL" ("missing: " + ($missing -join ", "))
}

$componentsBuild = Join-Path $BraveCore "components\BUILD.gn"
if (Test-Path $componentsBuild) {
  $text = Get-Content $componentsBuild -Raw
  if ($text -match '//brave/bnes:unit_tests') {
    Add-Result "bnes_gn_wire" "PASS" "components/BUILD.gn deps //brave/bnes:unit_tests"
  } else {
    Add-Result "bnes_gn_wire" "FAIL" "components/BUILD.gn missing //brave/bnes:unit_tests"
  }
} else {
  Add-Result "bnes_gn_wire" "FAIL" "components/BUILD.gn missing"
}

$bnesBuild = Join-Path $BnesDir "BUILD.gn"
if (Test-Path $bnesBuild) {
  $bg = Get-Content $bnesBuild -Raw
  $hasSec = $bg -match 'static_library\("security"\)'
  $hasUt = $bg -match 'source_set\("unit_tests"\)'
  if ($hasSec -and $hasUt) {
    Add-Result "bnes_build_gn" "PASS" "security + unit_tests targets declared"
  } else {
    Add-Result "bnes_build_gn" "FAIL" "BUILD.gn incomplete (need security + unit_tests)"
  }
} else {
  Add-Result "bnes_build_gn" "FAIL" "bnes/BUILD.gn missing"
}

# --- node_modules / init state ---
$nm = Join-Path $BraveCore "node_modules"
Add-Result "brave_node_modules" $(if (Test-Path $nm) { "PASS" } else { "FAIL" }) $(
  if (Test-Path $nm) { "present" } else { "missing — run pnpm install / pnpm run init inside brave-core (after layout ready)" }
)

# --- out dir / prior gn gen ---
$outDirs = @("out\Debug", "out\Component", "out\Release") | ForEach-Object { Join-Path $BraveCore $_ }
$anyOut = $false
foreach ($o in $outDirs) {
  if (Test-Path (Join-Path $o "build.ninja")) { $anyOut = $true; Add-Result "gn_gen_out" "PASS" $o; break }
}
if (-not $anyOut -and $foundSrc) {
  $outSrc = Join-Path $foundSrc "out"
  if (Test-Path $outSrc) {
    $ninjaFiles = Get-ChildItem $outSrc -Recurse -Filter build.ninja -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($ninjaFiles) {
      Add-Result "gn_gen_out" "PASS" $ninjaFiles.FullName
      $anyOut = $true
    }
  }
}
if (-not $anyOut) {
  Add-Result "gn_gen_out" "FAIL" "no build.ninja found — gn gen has not succeeded in this workspace"
}

# --- Summary ---
$fail = @($results | Where-Object { $_.status -eq "FAIL" })
$warn = @($results | Where-Object { $_.status -eq "WARN" })
$pass = @($results | Where-Object { $_.status -eq "PASS" })

Write-Host ""
Write-Host "=== summary ==="
Write-Host ("PASS={0} WARN={1} FAIL={2}" -f $pass.Count, $warn.Count, $fail.Count)
Write-Host "H5.2 complete criterion: chromium_src + depot_tools + gn + successful gn gen (build.ninja)"
Write-Host "NOTE: this script never claims Phase 1 or native bnes:// complete."

if ($fail.Count -eq 0) {
  Write-Host "RESULT: READY (all critical gates PASS) — may proceed to compile H5.3 targets"
  exit 0
}

Write-Host "RESULT: BLOCKED — H5.2 not complete. Failures:"
foreach ($f in $fail) { Write-Host ("  - {0}: {1}" -f $f.id, $f.detail) }
Write-Host "See docs/H5.2_BUILD_ENVIRONMENT.md for setup order."
exit 2
