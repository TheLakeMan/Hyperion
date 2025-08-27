# Hyperion AI Framework - Comprehensive Cleanup Script
# This script removes all build artifacts and caches to ensure a clean build environment.

Write-Host "========================================"
Write-Host "Hyperion AI Framework - Cleanup"
Write-Host "========================================"

# Get the script's directory
$scriptDir = $PSScriptRoot

# Remove build directories
$buildDirs = @("build", "build-test")
foreach ($dir in $buildDirs) {
    $fullPath = Join-Path $scriptDir $dir
    if (Test-Path $fullPath) {
        Write-Host "Removing directory: $fullPath"
        Remove-Item -Recurse -Force $fullPath
    }
}

# Remove object files
$objectFileExtensions = @("*.o", "*.obj")
$searchDirs = @("core", "tests", ".")
foreach ($dir in $searchDirs) {
    $fullPath = Join-Path $scriptDir $dir
    Get-ChildItem -Path $fullPath -Include $objectFileExtensions -Recurse | ForEach-Object {
        Write-Host "Removing file: $($_.FullName)"
        Remove-Item -Force $_.FullName
    }
}

# Remove executables from the root directory
Get-ChildItem -Path $scriptDir -Include "*.exe" -Exclude "cleanup_build.ps1" | ForEach-Object {
    Write-Host "Removing file: $($_.FullName)"
    Remove-Item -Force $_.FullName
}

Write-Host "========================================"
Write-Host "Cleanup completed successfully!"
Write-Host "========================================"