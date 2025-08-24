@echo off
REM Hyperion AI Framework - Quick Build Script for Windows
REM This script performs a fast development build with basic testing

setlocal enabledelayedexpansion

echo ========================================
echo Hyperion AI Framework - Quick Build
echo ========================================

REM Check if we're in the right directory
if not exist "CMakeLists.txt" (
    echo Error: CMakeLists.txt not found. Please run from project root.
    exit /b 1
)

REM Check for Visual Studio
where cl.exe >nul 2>&1
if errorlevel 1 (
    echo Setting up Visual Studio environment...
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" 2>nul
    if errorlevel 1 (
        call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" 2>nul
        if errorlevel 1 (
            echo Error: Visual Studio 2022 not found. Please install Visual Studio.
            exit /b 1
        )
    )
)

REM Create build directory
echo Creating build directory...
if exist "build\" rmdir /s /q "build\"
mkdir "build"
cd "build"

REM Configure with CMake
echo Configuring with CMake...
cmake -G "Visual Studio 17 2022" -A x64 ..
if errorlevel 1 (
    echo Error: CMake configuration failed.
    cd ..
    exit /b 1
)

REM Build the project
echo Building project (Release)...
cmake --build . --config Release --parallel
if errorlevel 1 (
    echo Error: Build failed.
    cd ..
    exit /b 1
)

REM Run quick tests
echo Running quick tests...
ctest -C Release --output-on-failure --timeout 60
if errorlevel 1 (
    echo Warning: Some tests failed. Check output above.
) else (
    echo All tests passed!
)

cd ..

echo ========================================
echo Quick build completed successfully!
echo ========================================
echo.
echo You can now:
echo   - Run CLI: .\build\Release\hyperion.exe --help
echo   - Start web server: .\build_webserver.bat
echo   - Run all tests: .\run_priority_tests.bat
echo   - See examples: .\examples\
echo.

pause