# Hyperion Build Configuration Validation Script (Windows PowerShell)
# Tests all CMake presets and build configurations on Windows

param(
    [switch]$Verbose,
    [string]$LogDir = "build-validation\logs"
)

# Script configuration
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
$BuildBaseDir = Join-Path $ProjectDir "build-validation"
$LogDirPath = Join-Path $BuildBaseDir "logs"

# Test results
$TotalTests = 0
$PassedTests = 0
$FailedTests = 0

# Colors for output
$Colors = @{
    Red = 'Red'
    Green = 'Green'
    Yellow = 'Yellow'
    Blue = 'Blue'
    White = 'White'
}

# Logging functions
function Write-Log {
    param([string]$Message, [string]$Color = 'White')
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logMessage = "[$timestamp] $Message"
    
    Write-Host $logMessage -ForegroundColor $Colors[$Color]
    Add-Content -Path (Join-Path $LogDirPath "validation.log") -Value $logMessage
}

function Write-Success {
    param([string]$Message)
    Write-Log "[PASS] $Message" 'Green'
    $script:PassedTests++
}

function Write-Error-Log {
    param([string]$Message)
    Write-Log "[FAIL] $Message" 'Red'
    $script:FailedTests++
}

function Write-Warning-Log {
    param([string]$Message)
    Write-Log "[WARN] $Message" 'Yellow'
}

# Setup function
function Setup-Validation {
    Write-Log "Setting up build validation environment..." 'Blue'
    
    # Create directories
    if (!(Test-Path $BuildBaseDir)) {
        New-Item -ItemType Directory -Path $BuildBaseDir -Force | Out-Null
    }
    if (!(Test-Path $LogDirPath)) {
        New-Item -ItemType Directory -Path $LogDirPath -Force | Out-Null
    }
    
    # Clear previous logs
    $logFile = Join-Path $LogDirPath "validation.log"
    if (Test-Path $logFile) {
        Clear-Content $logFile
    }
    
    # Print system information
    Write-Log "System Information:" 'Blue'
    Write-Log "  OS: $($env:OS)"
    Write-Log "  Architecture: $($env:PROCESSOR_ARCHITECTURE)"
    
    try {
        $cmakeVersion = & cmake --version 2>$null | Select-Object -First 1
        Write-Log "  CMake version: $cmakeVersion"
    } catch {
        Write-Log "  CMake: Not found"
    }
    
    try {
        $msvcVersion = & cl 2>$null | Select-Object -First 1
        Write-Log "  MSVC: $msvcVersion"
    } catch {
        Write-Log "  MSVC: Not found"
    }
    
    Set-Location $ProjectDir
}

# Test individual build configuration
function Test-BuildConfig {
    param(
        [string]$ConfigName,
        [string]$CmakeArgs
    )
    
    $script:TotalTests++
    $buildDir = Join-Path $BuildBaseDir $ConfigName
    
    Write-Log "Testing build configuration: $ConfigName" 'Blue'
    
    # Clean previous build
    if (Test-Path $buildDir) {
        Remove-Item -Recurse -Force $buildDir
    }
    New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
    
    # Configure
    Write-Log "  Configuring with: $CmakeArgs"
    $configureLog = Join-Path $LogDirPath "$ConfigName-configure.log"
    $configureCmd = "cmake -B `"$buildDir`" $CmakeArgs"
    
    try {
        Invoke-Expression $configureCmd > $configureLog 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Log "  ✓ Configuration successful"
        } else {
            Write-Error-Log "Configuration failed for $ConfigName"
            Write-Log "    See $configureLog for details"
            return $false
        }
    } catch {
        Write-Error-Log "Configuration failed for $ConfigName with exception: $($_.Exception.Message)"
        return $false
    }
    
    # Build
    Write-Log "  Building..."
    $buildLog = Join-Path $LogDirPath "$ConfigName-build.log"
    
    try {
        & cmake --build $buildDir > $buildLog 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Log "  ✓ Build successful"
        } else {
            Write-Error-Log "Build failed for $ConfigName"
            Write-Log "    See $buildLog for details"
            return $false
        }
    } catch {
        Write-Error-Log "Build failed for $ConfigName with exception: $($_.Exception.Message)"
        return $false
    }
    
    # Test if executables were created
    $examplePath = Join-Path $buildDir "examples\beginner_hello_world.exe"
    if (Test-Path $examplePath) {
        Write-Log "  ✓ Example executables created"
    } else {
        Write-Warning-Log "Example executables not found for $ConfigName"
    }
    
    Write-Success "Build configuration $ConfigName completed successfully"
    return $true
}

# Test CMake presets
function Test-CMakePresets {
    Write-Log "Testing CMake presets..." 'Blue'
    
    $presetsFile = Join-Path $ProjectDir "CMakePresets.json"
    if (!(Test-Path $presetsFile)) {
        Write-Warning-Log "CMakePresets.json not found"
        return
    }
    
    try {
        $presetsContent = Get-Content $presetsFile -Raw | ConvertFrom-Json
        $presets = $presetsContent.configurePresets | ForEach-Object { $_.name }
        
        foreach ($preset in $presets) {
            $script:TotalTests++
            Write-Log "Testing preset: $preset"
            
            $buildDir = Join-Path $BuildBaseDir "preset-$preset"
            if (Test-Path $buildDir) {
                Remove-Item -Recurse -Force $buildDir
            }
            
            $presetLog = Join-Path $LogDirPath "preset-$preset.log"
            
            try {
                & cmake --preset=$preset > $presetLog 2>&1
                if ($LASTEXITCODE -eq 0) {
                    & cmake --build $buildDir >> $presetLog 2>&1
                    if ($LASTEXITCODE -eq 0) {
                        Write-Success "Preset $preset built successfully"
                    } else {
                        Write-Error-Log "Preset $preset build failed"
                    }
                } else {
                    Write-Error-Log "Preset $preset configuration failed"
                }
            } catch {
                Write-Error-Log "Preset $preset failed with exception: $($_.Exception.Message)"
            }
        }
    } catch {
        Write-Warning-Log "Could not parse CMakePresets.json: $($_.Exception.Message)"
    }
}

# Test different build types
function Test-BuildTypes {
    Write-Log "Testing different build types..." 'Blue'
    
    $buildTypes = @("Debug", "Release", "RelWithDebInfo", "MinSizeRel")
    
    foreach ($buildType in $buildTypes) {
        Test-BuildConfig "build-type-$buildType" "-DCMAKE_BUILD_TYPE=$buildType -DBUILD_EXAMPLES=ON"
    }
}

# Test Visual Studio generators
function Test-VisualStudioGenerators {
    Write-Log "Testing Visual Studio generators..." 'Blue'
    
    # Check for Visual Studio installations
    $vsGenerators = @()
    
    try {
        $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        if (Test-Path $vsWhere) {
            $vsInstalls = & $vsWhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion
            foreach ($version in $vsInstalls) {
                if ($version -like "17.*") {
                    $vsGenerators += "Visual Studio 17 2022"
                } elseif ($version -like "16.*") {
                    $vsGenerators += "Visual Studio 16 2019"
                }
            }
        }
    } catch {
        Write-Warning-Log "Could not detect Visual Studio installations"
    }
    
    if ($vsGenerators.Count -eq 0) {
        $vsGenerators = @("Visual Studio 17 2022", "Visual Studio 16 2019")
    }
    
    foreach ($generator in $vsGenerators) {
        $generatorName = $generator -replace " ", "-"
        Test-BuildConfig "vs-$generatorName" "-G `"$generator`" -DBUILD_EXAMPLES=ON"
    }
}

# Test feature combinations
function Test-FeatureCombinations {
    Write-Log "Testing feature combinations..." 'Blue'
    
    # Test with all features enabled
    Test-BuildConfig "all-features" "-DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON -DBUILD_TESTS=ON -DENABLE_SIMD=ON"
    
    # Test minimal build
    Test-BuildConfig "minimal" "-DCMAKE_BUILD_TYPE=MinSizeRel -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF"
    
    # Test debug with memory tracking
    Test-BuildConfig "debug-memory" "-DCMAKE_BUILD_TYPE=Debug -DENABLE_MEMORY_DEBUGGING=ON -DBUILD_TESTS=ON"
}

# Test quick build scripts
function Test-QuickBuildScripts {
    Write-Log "Testing quick build scripts..." 'Blue'
    
    # Test Windows batch script
    $batchScript = Join-Path $ProjectDir "build_quick.bat"
    if (Test-Path $batchScript) {
        $script:TotalTests++
        Write-Log "Testing build_quick.bat..."
        
        $batchLog = Join-Path $LogDirPath "quick-build-bat.log"
        
        try {
            & cmd.exe /c $batchScript > $batchLog 2>&1
            if ($LASTEXITCODE -eq 0) {
                Write-Success "build_quick.bat executed successfully"
            } else {
                Write-Error-Log "build_quick.bat failed"
            }
        } catch {
            Write-Error-Log "build_quick.bat failed with exception: $($_.Exception.Message)"
        }
    }
}

# Run integration tests
function Test-Integration {
    Write-Log "Running integration tests..." 'Blue'
    
    # Use the most recent successful build
    $buildDir = Join-Path $BuildBaseDir "build-type-Release"
    
    if (!(Test-Path $buildDir)) {
        Write-Warning-Log "No release build available for integration testing"
        return
    }
    
    Set-Location $buildDir
    
    # Test example execution
    $examplePath = "examples\beginner_hello_world.exe"
    if (Test-Path $examplePath) {
        $script:TotalTests++
        Write-Log "Testing beginner example execution..."
        
        $integrationLog = Join-Path $LogDirPath "integration-beginner.log"
        
        try {
            $job = Start-Job -ScriptBlock { & $using:examplePath }
            Wait-Job $job -Timeout 30 | Out-Null
            $result = Receive-Job $job
            Remove-Job $job -Force
            
            $result | Out-File $integrationLog
            Write-Success "Beginner example executed successfully"
        } catch {
            Write-Error-Log "Beginner example execution failed: $($_.Exception.Message)"
        }
    }
    
    # Test memory validation if available
    $memoryTestPath = "tests\memory_validation_suite.exe"
    if (Test-Path $memoryTestPath) {
        $script:TotalTests++
        Write-Log "Running memory validation tests..."
        
        $memoryLog = Join-Path $LogDirPath "integration-memory.log"
        
        try {
            $job = Start-Job -ScriptBlock { & $using:memoryTestPath }
            Wait-Job $job -Timeout 120 | Out-Null
            $result = Receive-Job $job
            Remove-Job $job -Force
            
            $result | Out-File $memoryLog
            
            if ($job.State -eq "Completed") {
                Write-Success "Memory validation tests passed"
            } else {
                Write-Error-Log "Memory validation tests failed or timed out"
            }
        } catch {
            Write-Error-Log "Memory validation tests failed: $($_.Exception.Message)"
        }
    }
    
    Set-Location $ProjectDir
}

# Generate final report
function Generate-Report {
    Write-Log "Generating validation report..." 'Blue'
    
    $reportFile = Join-Path $LogDirPath "validation-report.txt"
    $successRate = if ($TotalTests -gt 0) { [math]::Round(($PassedTests * 100) / $TotalTests, 1) } else { 0 }
    
    $reportContent = @"
=== Hyperion Build Validation Report ===
Generated: $(Get-Date)
Project Directory: $ProjectDir

=== Summary ===
Total Tests: $TotalTests
Passed: $PassedTests
Failed: $FailedTests
Success Rate: $successRate%

=== System Information ===
OS: $($env:OS)
Architecture: $($env:PROCESSOR_ARCHITECTURE)
PowerShell: $($PSVersionTable.PSVersion)

=== Detailed Results ===
See individual log files in $LogDirPath for detailed information:
- validation.log: Complete execution log
- *-configure.log: Configuration phase logs
- *-build.log: Build phase logs
- integration-*.log: Integration test logs

=== Recommendations ===
"@

    if ($FailedTests -eq 0) {
        $reportContent += "`n✅ All tests passed! The build system is working correctly."
    } elseif ($FailedTests -lt 3) {
        $reportContent += "`n⚠️  Some tests failed, but overall build system appears functional."
        $reportContent += "`n   Review failed test logs for specific issues."
    } else {
        $reportContent += "`n❌ Multiple test failures detected. Build system needs attention."
        $reportContent += "`n   Priority: Fix configuration and compilation issues first."
    }
    
    $reportContent | Out-File $reportFile -Encoding UTF8
    Write-Host $reportContent
    Write-Log "Validation report saved to: $reportFile"
}

# Main execution
function Main {
    Write-Log "=== Hyperion Build Configuration Validation ===" 'Blue'
    Write-Log "Starting comprehensive build validation..." 'Blue'
    
    Setup-Validation
    
    # Run all validation tests
    Test-CMakePresets
    Test-BuildTypes
    Test-VisualStudioGenerators
    Test-FeatureCombinations
    Test-QuickBuildScripts
    Test-Integration
    
    # Generate final report
    Generate-Report
    
    # Exit with appropriate code
    if ($FailedTests -eq 0) {
        Write-Success "=== All validation tests completed successfully! ==="
        exit 0
    } else {
        Write-Error-Log "=== Validation completed with $FailedTests failed tests ==="
        exit 1
    }
}

# Run main function
Main