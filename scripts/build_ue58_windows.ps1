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
$UnrealBuildTool = Join-Path $EngineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
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
if (-not (Test-Path $BuildScript -PathType Leaf)) {
    Stop-Build "Build.bat not found: $BuildScript" 5
}

$ProjectJson = Get-Content $ProjectFile -Raw
if ($ProjectJson -notmatch '"EngineAssociation"\s*:\s*"5\.8"') {
    Stop-Build "WorldSimDemo.uproject is not associated with UE 5.8." 6
}

$VsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $VsWhere -PathType Leaf)) {
    Stop-Build "Visual Studio Installer vswhere.exe was not found. Install Visual Studio 2026 or Visual Studio 2022 17.14+." 7
}

$VisualStudio = & $VsWhere -latest -version "[18.0,19.0)" -products * `
    -requires Microsoft.VisualStudio.Workload.NativeGame -property installationPath
$VisualStudioLabel = "Visual Studio 2026"
if ([string]::IsNullOrWhiteSpace($VisualStudio)) {
    $VisualStudio = & $VsWhere -latest -version "[17.14,18.0)" -products * `
        -requires Microsoft.VisualStudio.Workload.NativeGame -property installationPath
    $VisualStudioLabel = "Visual Studio 2022 17.14+"
}
if ([string]::IsNullOrWhiteSpace($VisualStudio)) {
    Stop-Build "Visual Studio 2026 or Visual Studio 2022 17.14+ with the Game development with C++ workload was not found." 8
}

New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null

Write-Host "Project: $ProjectFile"
Write-Host "Engine:  $EngineRoot"
Write-Host "$VisualStudioLabel`: $VisualStudio"

if (-not $SkipGenerate) {
    if (Test-Path $GenerateScript -PathType Leaf) {
        Write-Host "Generating Visual Studio project files with GenerateProjectFiles.bat..."
        & $GenerateScript "-project=$ProjectFile" -game -engine 2>&1 | Tee-Object -FilePath $GenerateLog
        $GenerateExitCode = $LASTEXITCODE
        if ($GenerateExitCode -ne 0) {
            Stop-Build "Project generation failed with exit code $GenerateExitCode. Log: $GenerateLog" $GenerateExitCode
        }
    }
    elseif (Test-Path $UnrealBuildTool -PathType Leaf) {
        Write-Host "GenerateProjectFiles.bat is unavailable; using UnrealBuildTool -ProjectFiles..."
        & $UnrealBuildTool -ProjectFiles "-Project=$ProjectFile" -Game -Rocket -Progress 2>&1 `
            | Tee-Object -FilePath $GenerateLog
        $GenerateExitCode = $LASTEXITCODE
        if ($GenerateExitCode -ne 0) {
            Stop-Build "UnrealBuildTool project generation failed with exit code $GenerateExitCode. Log: $GenerateLog" $GenerateExitCode
        }
    }
    else {
        Write-Warning "No project-file generator was found. Continuing with Build.bat; solution generation is not required for this build."
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
