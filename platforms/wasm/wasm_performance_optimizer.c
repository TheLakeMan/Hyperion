/**
 * WASM Performance Optimizer
 * 
 * Browser-specific performance optimizations for Hyperion WebAssembly
 */

#include "wasm_performance_optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <emscripten.h>

/* Performance tracking */
typedef struct {
    double total_inference_time;
    int inference_count;
    double average_tokens_per_second;
    int optimization_level;
} WasmPerformanceTracker;

static WasmPerformanceTracker g_perf = {0};

/**
 * Initialize performance optimization
 */
void wasm_perf_init(void) {
    printf("Initializing WASM performance optimization...\n");
    
    g_perf.optimization_level = 1; // Default optimization level
    
    // Detect browser capabilities
    EM_ASM({
        // Check for high-resolution timing
        if (typeof performance !== 'undefined' && performance.now) {
            Module.hasHighResTimer = true;
        }
        
        // Check for WebWorker support
        if (typeof Worker !== 'undefined') {
            Module.hasWebWorkers = true;
        }
        
        // Check for SharedArrayBuffer support
        if (typeof SharedArrayBuffer !== 'undefined') {
            Module.hasSharedArrayBuffer = true;
        }
        
        // Check for WebAssembly SIMD support
        if (typeof WebAssembly !== 'undefined' && WebAssembly.validate) {
            try {
                // Test SIMD instruction
                var simdTest = new Uint8Array([0, 97, 115, 109, 1, 0, 0, 0]);
                Module.hasSIMD = WebAssembly.validate(simdTest);
            } catch (e) {
                Module.hasSIMD = false;
            }
        }
        
        console.log('Browser capabilities detected:');
        console.log('  High-res timer:', Module.hasHighResTimer);
        console.log('  WebWorkers:', Module.hasWebWorkers);
        console.log('  SharedArrayBuffer:', Module.hasSharedArrayBuffer);
        console.log('  WASM SIMD:', Module.hasSIMD);
    });
}

/**
 * Start performance measurement
 */
double wasm_perf_start_timer(void) {
    return emscripten_get_now();
}

/**
 * End performance measurement and record
 */
void wasm_perf_end_timer(double start_time, int tokens_generated) {
    double end_time = emscripten_get_now();
    double duration = end_time - start_time;
    
    g_perf.total_inference_time += duration;
    g_perf.inference_count++;
    
    if (tokens_generated > 0) {
        double tokens_per_second = tokens_generated * 1000.0 / duration;
        g_perf.average_tokens_per_second = 
            (g_perf.average_tokens_per_second * (g_perf.inference_count - 1) + tokens_per_second) / 
            g_perf.inference_count;
        
        printf("Inference completed: %.2f ms, %d tokens, %.2f tokens/sec\n",
               duration, tokens_generated, tokens_per_second);
    }
}

/**
 * Optimize for browser environment
 */
void wasm_perf_optimize_for_browser(void) {
    printf("Applying browser-specific optimizations...\n");
    
    EM_ASM({
        // Request high performance mode
        if (typeof navigator !== 'undefined' && navigator.hardwareConcurrency) {
            console.log('CPU cores available:', navigator.hardwareConcurrency);
        }
        
        // Optimize for animation frame timing
        if (typeof requestAnimationFrame !== 'undefined') {
            console.log('Animation frame optimization available');
        }
        
        // Check memory pressure
        if (typeof performance !== 'undefined' && performance.memory) {
            var memoryPressure = performance.memory.usedJSHeapSize / performance.memory.jsHeapSizeLimit;
            if (memoryPressure > 0.8) {
                console.warn('High memory pressure detected:', (memoryPressure * 100).toFixed(1) + '%');
                Module.highMemoryPressure = true;
            }
        }
        
        // Set up idle callback for background processing
        if (typeof requestIdleCallback !== 'undefined') {
            Module.hasIdleCallback = true;
            console.log('Idle callback optimization available');
        }
    });
}

/**
 * Adaptive performance tuning based on device capabilities
 */
void wasm_perf_auto_tune(void) {
    printf("Auto-tuning performance parameters...\n");
    
    int device_class = 0; // 0=low-end, 1=mid-range, 2=high-end
    
    EM_ASM({
        var deviceClass = 0;
        
        // Estimate device class based on available information
        if (typeof navigator !== 'undefined') {
            var cores = navigator.hardwareConcurrency || 1;
            var memory = navigator.deviceMemory || 1; // GB
            
            if (cores >= 8 && memory >= 8) {
                deviceClass = 2; // High-end
            } else if (cores >= 4 && memory >= 4) {
                deviceClass = 1; // Mid-range
            } else {
                deviceClass = 0; // Low-end
            }
            
            console.log('Device classification:', 
                       deviceClass === 2 ? 'High-end' : 
                       deviceClass === 1 ? 'Mid-range' : 'Low-end');
        }
        
        // Check if mobile device
        var isMobile = /Android|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent);
        if (isMobile && deviceClass > 0) {
            deviceClass--; // Reduce classification for mobile
            console.log('Mobile device detected, adjusting performance class');
        }
        
        Module.deviceClass = deviceClass;
    });
    
    device_class = EM_ASM_INT({
        return Module.deviceClass || 0;
    });
    
    // Adjust optimization level based on device class
    switch (device_class) {
        case 0: // Low-end
            g_perf.optimization_level = 0;
            printf("Optimizing for low-end device: minimal features, maximum efficiency\n");
            break;
            
        case 1: // Mid-range
            g_perf.optimization_level = 1;
            printf("Optimizing for mid-range device: balanced performance\n");
            break;
            
        case 2: // High-end
            g_perf.optimization_level = 2;
            printf("Optimizing for high-end device: maximum features enabled\n");
            break;
    }
}

/**
 * Background processing using idle callbacks
 */
void wasm_perf_schedule_background_task(void (*callback)(void)) {
    EM_ASM({
        if (Module.hasIdleCallback && typeof requestIdleCallback !== 'undefined') {
            requestIdleCallback(function(deadline) {
                if (deadline.timeRemaining() > 5) { // At least 5ms available
                    try {
                        // Call the C function
                        if (typeof Module._wasm_perf_background_callback !== 'undefined') {
                            Module._wasm_perf_background_callback();
                        }
                    } catch (e) {
                        console.error('Background task error:', e);
                    }
                }
            });
        } else {
            // Fallback to setTimeout
            setTimeout(function() {
                try {
                    if (typeof Module._wasm_perf_background_callback !== 'undefined') {
                        Module._wasm_perf_background_callback();
                    }
                } catch (e) {
                    console.error('Background task error:', e);
                }
            }, 16); // ~60fps
        }
    });
}

/**
 * Get recommended parameters for current device
 */
WasmPerfRecommendations wasm_perf_get_recommendations(void) {
    WasmPerfRecommendations rec = {0};
    
    switch (g_perf.optimization_level) {
        case 0: // Low-end device
            rec.max_tokens = 50;
            rec.memory_limit_mb = 16;
            rec.use_streaming = 1;
            rec.batch_size = 1;
            rec.enable_caching = 0;
            break;
            
        case 1: // Mid-range device
            rec.max_tokens = 100;
            rec.memory_limit_mb = 32;
            rec.use_streaming = 1;
            rec.batch_size = 2;
            rec.enable_caching = 1;
            break;
            
        case 2: // High-end device
            rec.max_tokens = 200;
            rec.memory_limit_mb = 64;
            rec.use_streaming = 0;
            rec.batch_size = 4;
            rec.enable_caching = 1;
            break;
    }
    
    // Check memory pressure and adjust
    int high_memory_pressure = EM_ASM_INT({
        return Module.highMemoryPressure ? 1 : 0;
    });
    
    if (high_memory_pressure) {
        rec.memory_limit_mb /= 2;
        rec.max_tokens /= 2;
        rec.use_streaming = 1;
        printf("High memory pressure detected, reducing recommendations\n");
    }
    
    return rec;
}

/**
 * Print performance statistics
 */
void wasm_perf_print_stats(void) {
    printf("=== WASM Performance Statistics ===\n");
    printf("Inference count: %d\n", g_perf.inference_count);
    
    if (g_perf.inference_count > 0) {
        printf("Average inference time: %.2f ms\n", 
               g_perf.total_inference_time / g_perf.inference_count);
        printf("Average tokens/second: %.2f\n", g_perf.average_tokens_per_second);
    }
    
    printf("Optimization level: %d\n", g_perf.optimization_level);
    
    // Browser-specific performance info
    EM_ASM({
        console.log('=== Browser Performance Info ===');
        
        if (typeof performance !== 'undefined' && performance.memory) {
            console.log('JS Heap usage:', 
                       (performance.memory.usedJSHeapSize / 1024 / 1024).toFixed(2), 'MB');
        }
        
        if (typeof performance !== 'undefined' && performance.timing) {
            console.log('Page load time:', 
                       performance.timing.loadEventEnd - performance.timing.navigationStart, 'ms');
        }
        
        // FPS estimation (if available)
        if (typeof performance !== 'undefined' && performance.now) {
            var lastTime = performance.now();
            var frameCount = 0;
            
            function measureFPS() {
                frameCount++;
                if (frameCount === 60) {
                    var now = performance.now();
                    var fps = 60000 / (now - lastTime);
                    console.log('Estimated FPS:', fps.toFixed(1));
                }
            }
            
            // Measure for a few frames
            for (var i = 0; i < 5; i++) {
                requestAnimationFrame(measureFPS);
            }
        }
    });
}

/**
 * Reset performance statistics
 */
void wasm_perf_reset_stats(void) {
    printf("Resetting WASM performance statistics\n");
    g_perf.total_inference_time = 0;
    g_perf.inference_count = 0;
    g_perf.average_tokens_per_second = 0;
}

/**
 * Cleanup performance optimizer
 */
void wasm_perf_cleanup(void) {
    printf("Cleaning up WASM performance optimizer\n");
    wasm_perf_print_stats();
    wasm_perf_reset_stats();
}