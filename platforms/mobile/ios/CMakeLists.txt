# iOS Build Configuration for Hyperion
cmake_minimum_required(VERSION 3.19)

project(hyperion_ios)

# iOS-specific settings
if(IOS)
    message(STATUS "Building Hyperion for iOS")
    
    # Set iOS deployment target
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "iOS Deployment Target")
    set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "12.0")
    
    # iOS-specific compiler flags
    set(IOS_COMPILE_FLAGS
        "-DIOS"
        "-DHYPERION_IOS=1"
        "-DHYPERION_MOBILE=1"
        "-DHYPERION_NO_FILESYSTEM=1"
        "-ffast-math"
        "-ftree-vectorize"
        "-fomit-frame-pointer"
        "-fembed-bitcode"
    )
    
    # Architecture-specific optimizations
    if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
        list(APPEND IOS_COMPILE_FLAGS "-march=armv8-a" "-mtune=apple-a12")
        set(HYPERION_SIMD_SUPPORT ON)
        add_definitions(-DHYPERION_NEON=1)
        message(STATUS "Building for iOS ARM64 with NEON support")
    elseif(CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
        list(APPEND IOS_COMPILE_FLAGS "-march=x86-64" "-msse4.2")
        set(HYPERION_SIMD_SUPPORT ON)
        add_definitions(-DHYPERION_SSE=1)
        message(STATUS "Building for iOS Simulator x86_64 with SSE support")
    endif()
    
    # Apply compiler flags
    string(REPLACE ";" " " IOS_FLAGS_STR "${IOS_COMPILE_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IOS_FLAGS_STR}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IOS_FLAGS_STR}")
    
    # iOS framework-specific settings
    set(CMAKE_FRAMEWORK_PATH ${CMAKE_FRAMEWORK_PATH} ${CMAKE_OSX_SYSROOT}/System/Library/Frameworks)
    
    # Find iOS frameworks
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(METAL_FRAMEWORK Metal)
    find_library(METALKIT_FRAMEWORK MetalKit)
    find_library(COREML_FRAMEWORK CoreML)
    find_library(ACCELERATE_FRAMEWORK Accelerate)
    
    # iOS-specific source files
    set(HYPERION_IOS_SOURCES
        ../../../core/memory.c
        ../../../core/config.c
        ../../../models/text/generate.c
        ../../../models/text/tokenizer.c
        ../../../utils/tensor.c
    )
    
    # iOS-specific Swift files
    set(HYPERION_IOS_SWIFT_SOURCES
        HyperionAI.swift
    )
    
    # Create iOS framework
    add_library(HyperionAI STATIC
        ${HYPERION_IOS_SOURCES}
    )
    
    target_include_directories(HyperionAI PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/models
        ${CMAKE_SOURCE_DIR}/utils
        ${CMAKE_SOURCE_DIR}/interface
    )
    
    # iOS-specific preprocessor definitions
    target_compile_definitions(HyperionAI PRIVATE
        IOS=1
        HYPERION_IOS=1
        HYPERION_MOBILE=1
        HYPERION_MINIMAL_MEMORY=1
        HYPERION_4BIT_QUANTIZATION=1
        HYPERION_METAL_ACCELERATION=1
    )
    
    # Link iOS frameworks
    target_link_libraries(HyperionAI
        ${FOUNDATION_FRAMEWORK}
        ${METAL_FRAMEWORK}
        ${METALKIT_FRAMEWORK}
        ${COREML_FRAMEWORK}
        ${ACCELERATE_FRAMEWORK}
    )
    
    # Set framework properties
    set_target_properties(HyperionAI PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER com.hyperion.ai
        MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        MACOSX_FRAMEWORK_SHORT_VERSION_STRING "1.0"
        MACOSX_FRAMEWORK_BUNDLE_VERSION "1.0.0"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ""
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"  # iPhone and iPad
        PUBLIC_HEADER hyperion_ios.h
    )
    
    # Create umbrella header
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/hyperion_ios.h 
"//
//  HyperionAI.h
//  Hyperion AI Framework for iOS
//
//  Ultra-lightweight AI inference for mobile devices
//

#import <Foundation/Foundation.h>

//! Project version number for HyperionAI.
FOUNDATION_EXPORT double HyperionAIVersionNumber;

//! Project version string for HyperionAI.
FOUNDATION_EXPORT const unsigned char HyperionAIVersionString[];

// Core Hyperion headers
#include \"memory.h\"
#include \"config.h\"
#include \"generate.h\"
#include \"tokenizer.h\"

// iOS-specific interface
@interface HyperionAI : NSObject

+ (instancetype)sharedInstance;
- (BOOL)initializeWithConfig:(nullable NSString *)configPath error:(NSError **)error;
- (nullable NSString *)generateTextFromPrompt:(NSString *)prompt 
                                   maxTokens:(NSInteger)maxTokens 
                                 temperature:(float)temperature 
                                        topK:(NSInteger)topK
                                       error:(NSError **)error;
- (float)memoryUsageInMB;
- (NSString *)performanceStats;
- (BOOL)isInitialized;
- (void)setConfigValue:(NSString *)value forKey:(NSString *)key;
- (void)cleanup;

@end
")
    
    # Copy header to framework
    add_custom_command(TARGET HyperionAI POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/hyperion_ios.h
        $<TARGET_FILE_DIR:HyperionAI>/Headers/
    )
    
    # Create Info.plist
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>HyperionAI</string>
    <key>CFBundleIdentifier</key>
    <string>com.hyperion.ai</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>HyperionAI</string>
    <key>CFBundlePackageType</key>
    <string>FMWK</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>NSPrincipalClass</key>
    <string></string>
    <key>CFBundleSupportedPlatforms</key>
    <array>
        <string>iPhoneOS</string>
    </array>
    <key>MinimumOSVersion</key>
    <string>12.0</string>
    <key>UIDeviceFamily</key>
    <array>
        <integer>1</integer>
        <integer>2</integer>
    </array>
</dict>
</plist>
")
    
    # Custom target for Xcode integration
    add_custom_target(ios-integration
        COMMAND ${CMAKE_COMMAND} -E echo "=== iOS Integration Instructions ==="
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "1. Add HyperionAI.framework to your Xcode project"
        COMMAND ${CMAKE_COMMAND} -E echo "2. Link framework in Build Phases > Link Binary With Libraries"
        COMMAND ${CMAKE_COMMAND} -E echo "3. Add framework search path in Build Settings"
        COMMAND ${CMAKE_COMMAND} -E echo "4. Import in your Swift/Objective-C code:"
        COMMAND ${CMAKE_COMMAND} -E echo "   import HyperionAI  // Swift"
        COMMAND ${CMAKE_COMMAND} -E echo "   #import <HyperionAI/HyperionAI.h>  // Objective-C"
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "Swift Usage Example:"
        COMMAND ${CMAKE_COMMAND} -E echo "  let hyperion = HyperionAI.shared()"
        COMMAND ${CMAKE_COMMAND} -E echo "  hyperion.initialize(withConfig: nil)"
        COMMAND ${CMAKE_COMMAND} -E echo "  let result = hyperion.generateText(fromPrompt: \"Hello\", maxTokens: 50, temperature: 0.7, topK: 40)"
        DEPENDS HyperionAI
        COMMENT "Displaying iOS integration instructions"
    )
    
    # Code signing target (requires Apple Developer account)
    add_custom_target(ios-sign
        COMMAND codesign --force --sign "iPhone Developer" $<TARGET_FILE:HyperionAI>
        DEPENDS HyperionAI
        COMMENT "Code signing iOS framework"
    )
    
    # Create universal framework (device + simulator)
    add_custom_target(ios-universal
        COMMAND ${CMAKE_COMMAND} -E echo "Building universal iOS framework..."
        COMMAND xcodebuild -project ${CMAKE_BINARY_DIR}/hyperion_ios.xcodeproj -scheme HyperionAI -configuration Release -arch arm64 BUILD_DIR=${CMAKE_BINARY_DIR}/device-build
        COMMAND xcodebuild -project ${CMAKE_BINARY_DIR}/hyperion_ios.xcodeproj -scheme HyperionAI -configuration Release -arch x86_64 -sdk iphonesimulator BUILD_DIR=${CMAKE_BINARY_DIR}/sim-build
        COMMAND lipo -create ${CMAKE_BINARY_DIR}/device-build/Release-iphoneos/HyperionAI.framework/HyperionAI ${CMAKE_BINARY_DIR}/sim-build/Release-iphonesimulator/HyperionAI.framework/HyperionAI -output ${CMAKE_BINARY_DIR}/HyperionAI-universal
        DEPENDS HyperionAI
        COMMENT "Creating universal iOS framework"
    )
    
else()
    message(WARNING "iOS build requires iOS toolchain and Xcode")
endif()

# Build instructions and usage
add_custom_target(ios-help
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Hyperion iOS Build ==="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Prerequisites:"
    COMMAND ${CMAKE_COMMAND} -E echo "  1. macOS with Xcode installed"
    COMMAND ${CMAKE_COMMAND} -E echo "  2. iOS SDK and deployment tools"
    COMMAND ${CMAKE_COMMAND} -E echo "  3. Apple Developer account (for device deployment)"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Build Commands:"
    COMMAND ${CMAKE_COMMAND} -E echo "  # Device build"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake .. -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build . --config Release"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "  # Simulator build"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake .. -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_SYSROOT=iphonesimulator"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build . --config Release"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Features:"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Native iOS framework"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Metal acceleration support"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Core ML integration"
    COMMAND ${CMAKE_COMMAND} -E echo "  - NEON SIMD optimization"
    COMMAND ${CMAKE_COMMAND} -E echo "  - Ultra-low memory footprint"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMENT "Displaying iOS build help"
)