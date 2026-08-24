param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [switch]$SkipGenerate
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Stop-Build([string]$Message, [int]$ExitCode = 1) {
    Write-Error $Message
    exit $ExitCode
}

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ProjectFile = Join-Path $ProjectRoot "WorldSimDemo.uproject"
$GenerateScript = Join-Path $EngineRoot "Engine\Build\BatchFiles\GenerateProjectFiles.bat"
$BuildScript = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"
$LogDirectory = Join-Path $ProjectRoot "Saved\BuildLogs"
$Timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$GenerateLog = Join-Path $LogDirectory "generate-$Timestamp.log"
$BuildLog = Join-Path $LogDirectory "build-$Timestamp.log"

if (-not (Test-Path $ProjectFile -PathType Leaf)) {
    Stop-Build "Project file not found: $ProjectFile" 2
}
if (-not (Test-Path $EngineRoot -PathType Container)) {
    Stop-Build "UE 5.8 root not found: $EngineRoot. Pass -EngineRoot with the installed path." 3
}
if (-not (Test-Path $GenerateScript -PathType Leaf)) {
    Stop-Build "GenerateProjectFiles.bat not found: $GenerateScript" 4
}
if (-not (Test-Path $BuildScript -PathType Leaf)) {
    Stop-Build "Build.bat not found: $BuildScript" 5
}

$ProjectJson = Get-Content $ProjectFile -Raw
if ($ProjectJson -notmatch '"EngineAssociation"\s*:\s*"5\.8"') {
    Stop-Build "WorldSimDemo.uproject is not associated with UE 5.8." 6
}

$VsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $VsWhere -PathType Leaf)) {
    Stop-Build "Visual Studio Installer vswhere.exe was not found. Install Visual Studio 2022." 7
}

$VisualStudio = & $VsWhere -latest -version "[17.0,18.0)" -products * `
    -requires Microsoft.VisualStudio.Workload.NativeGame -property installationPath
if ([string]::IsNullOrWhiteSpace($VisualStudio)) {
    Stop-Build "Visual Studio 2022 with the Game development with C++ workload was not found." 8
}

New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null

Write-Host "Project: $ProjectFile"
Write-Host "Engine:  $EngineRoot"
Write-Host "VS 2022: $VisualStudio"

if (-not $SkipGenerate) {
    Write-Host "Generating Visual Studio project files..."
    & $GenerateScript "-project=$ProjectFile" -game -engine 2>&1 | Tee-Object -FilePath $GenerateLog
    $GenerateExitCode = $LASTEXITCODE
    if ($GenerateExitCode -ne 0) {
        Stop-Build "Project generation failed with exit code $GenerateExitCode. Log: $GenerateLog" $GenerateExitCode
    }
}

Write-Host "Building WorldSimDemoEditor Win64 Development..."
& $BuildScript WorldSimDemoEditor Win64 Development "-Project=$ProjectFile" `
    -WaitMutex -NoHotReloadFromIDE 2>&1 | Tee-Object -FilePath $BuildLog
$BuildExitCode = $LASTEXITCODE
if ($BuildExitCode -ne 0) {
    Stop-Build "Editor build failed with exit code $BuildExitCode. Log: $BuildLog" $BuildExitCode
}

Write-Host "UE 5.8 Editor build succeeded."
Write-Host "Build log: $BuildLog"
exit 0
