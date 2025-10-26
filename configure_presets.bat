@echo off
REM Configure CMake with Windows Debug Preset for Hyperion

echo Configuring Hyperion with Windows Debug preset...

REM Clean up the build directory
rmdir /s /q build 2>nul

REM Configure with Windows Debug preset
cmake --preset windows-debug

if %ERRORLEVEL% EQU 0 (
    echo SUCCESS: CMake configured with windows-debug preset
    echo Build directory: build/windows-debug
    echo.
    echo To build the project, run:
    echo cmake --build --preset windows-debug
    echo.
    echo To test the project, run:
    echo ctest --preset windows-debug
) else (
    echo ERROR: Failed to configure with windows-debug preset
    echo Error level: %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)