# JustLive Build Script
# Compiles the JustLive project using Unreal Engine 5.6

param(
    [string]$Configuration = "Development Editor",
    [string]$Platform = "Win64",
    [switch]$Clean,
    [switch]$Rebuild
)

$ErrorActionPreference = "Stop"

# Paths
$UE_PATH = "D:/Applications/UE_5.6"
$PROJECT_ROOT = $PSScriptRoot
$PROJECT_FILE = Join-Path $PROJECT_ROOT "JustLive.uproject"
$UBT_PATH = Join-Path $UE_PATH "Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe"

# Validate paths
if (-not (Test-Path $UE_PATH)) {
    Write-Error "Unreal Engine not found at: $UE_PATH"
    exit 1
}

if (-not (Test-Path $PROJECT_FILE)) {
    Write-Error "Project file not found at: $PROJECT_FILE"
    exit 1
}

if (-not (Test-Path $UBT_PATH)) {
    Write-Error "UnrealBuildTool not found at: $UBT_PATH"
    exit 1
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "JustLive Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Configuration: $Configuration" -ForegroundColor Yellow
Write-Host "Platform: $Platform" -ForegroundColor Yellow
Write-Host "Project: $PROJECT_FILE" -ForegroundColor Yellow
Write-Host ""

# Clean if requested
if ($Clean -or $Rebuild) {
    Write-Host "Cleaning build artifacts..." -ForegroundColor Yellow
    
    $DirsToClean = @(
        (Join-Path $PROJECT_ROOT "Binaries"),
        (Join-Path $PROJECT_ROOT "Intermediate"),
        (Join-Path $PROJECT_ROOT "Saved/Logs")
    )
    
    foreach ($dir in $DirsToClean) {
        if (Test-Path $dir) {
            Write-Host "  Removing: $dir" -ForegroundColor Gray
            Remove-Item -Path $dir -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
    Write-Host "Clean complete!" -ForegroundColor Green
    Write-Host ""
}

# Build the project
Write-Host "Building JustLive..." -ForegroundColor Yellow
Write-Host ""

$BuildArgs = @(
    "JustLiveEditor",
    $Platform,
    "Development",
    "-Project=`"$PROJECT_FILE`"",
    "-Progress",
    "-NoHotReloadFromIDE"
)

Write-Host "Running: $UBT_PATH $($BuildArgs -join ' ')" -ForegroundColor Gray
Write-Host ""

& $UBT_PATH $BuildArgs

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "BUILD SUCCESSFUL!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Red
    Write-Host "BUILD FAILED!" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Red
    exit $LASTEXITCODE
}
