param(
  [Parameter(Mandatory=$false)]
  [string]$JuceDir = "C:\JUCE",
  [ValidateSet("Release","Debug")]
  [string]$Configuration = "Release",
  [switch]$InstallToUserVst3
)
$ErrorActionPreference = "Stop"
$ProjectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $ProjectDir "build-win"

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
  throw "CMake was not found. Install CMake 3.22+ and reopen PowerShell."
}
if (-not (Test-Path (Join-Path $JuceDir "CMakeLists.txt"))) {
  throw "JUCE was not found at '$JuceDir'. Download JUCE and pass -JuceDir 'C:\path\to\JUCE'."
}

Write-Host "Configuring Wonder Melody Generator using JUCE at $JuceDir ..." -ForegroundColor Cyan
cmake -S $ProjectDir -B $BuildDir -DJUCE_DIR="$JuceDir" -A x64
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed (exit $LASTEXITCODE)." }

Write-Host "Building $Configuration ..." -ForegroundColor Cyan
cmake --build $BuildDir --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { throw "Build failed (exit $LASTEXITCODE). Review the compiler output above." }

$ArtifactRoot = Join-Path $BuildDir "WonderMelodyGenerator_artefacts\$Configuration\VST3"
$Vst3 = Join-Path $ArtifactRoot "Wonder Melody Generator.vst3"
if (-not (Test-Path $Vst3)) {
  $Vst3 = Get-ChildItem -Path (Join-Path $BuildDir "WonderMelodyGenerator_artefacts") -Filter "*.vst3" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
}
if (-not $Vst3 -or -not (Test-Path $Vst3)) {
  throw "Build finished but the VST3 bundle could not be located. Inspect $BuildDir\WonderMelodyGenerator_artefacts."
}
Write-Host "Build artifact: $Vst3" -ForegroundColor Green

if ($InstallToUserVst3) {
  $InstallDir = Join-Path $env:LOCALAPPDATA "Programs\Common\VST3"
  New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
  $Destination = Join-Path $InstallDir (Split-Path $Vst3 -Leaf)
  if (Test-Path $Destination) { Remove-Item -Recurse -Force $Destination }
  Copy-Item -Recurse -Force $Vst3 $Destination
  Write-Host "Installed to: $Destination" -ForegroundColor Green
  Write-Host "Now open FL Studio, open Plugin Manager, and run a plugin scan." -ForegroundColor Yellow
}
