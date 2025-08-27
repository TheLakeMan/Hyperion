@echo off
echo Hyperion Cleanup Script - Preparing for GitHub Publishing
echo ------------------------------------------------
echo.

REM Remove build directory
if exist build\ (
    echo Removing build directory...
    rmdir /s /q build
    echo Build directory removed
) else (
    echo Build directory not found (already clean)
)

REM Remove VS2022 directory
if exist vs2022\ (
    echo Removing vs2022 directory...
    rmdir /s /q vs2022
    echo VS2022 directory removed
) else (
    echo VS2022 directory not found (already clean)
)

REM NOTE: The batch file version has limited functionality compared to the PowerShell script
REM For a more comprehensive cleanup, please run the PowerShell script: cleanup_build.ps1

echo.
echo Cleanup of major build directories complete!
echo For more thorough cleanup of individual files, please run the PowerShell script:
echo   powershell -ExecutionPolicy Bypass -File cleanup_build.ps1
echo.
echo Your project is now cleaner for GitHub publishing.