/**
 * Hyperion WASM Web Worker
 * 
 * Handles background AI processing to keep the main UI thread responsive.
 * This worker can perform heavy computations without blocking user interactions.
 */

// Import WASM module in worker context
importScripts('hyperion_wasm.js');

let hyperionModule = null;
let initialized = false;

// Worker message handler
self.onmessage = function(event) {
    const { task, data } = event.data;
    
    switch (task) {
        case 'init':
            initializeHyperion(data);
            break;
            
        case 'generate':
            generateText(data);
            break;
            
        case 'cleanup':
            cleanup();
            break;
            
        default:
            postMessage({
                task,
                error: `Unknown task: ${task}`
            });
    }
};

// Initialize Hyperion in worker
async function initializeHyperion(config = {}) {
    try {
        // Load WASM module
        hyperionModule = await HyperionModule({
            locateFile: (path, prefix) => {
                // Adjust path for worker context
                return prefix + path;
            }
        });
        
        // Initialize Hyperion
        const result = hyperionModule.ccall('hyperion_wasm_init', 'number', [], []);
        
        if (result === 0) {
            initialized = true;
            postMessage({
                task: 'init',
                result: { success: true, message: 'Worker initialized successfully' }
            });
        } else {
            throw new Error(`Worker initialization failed with code: ${result}`);
        }
        
    } catch (error) {
        postMessage({
            task: 'init',
            error: error.message
        });
    }
}

// Generate text in background
function generateText(options) {
    if (!initialized || !hyperionModule) {
        postMessage({
            task: 'generate',
            error: 'Worker not initialized'
        });
        return;
    }
    
    try {
        const startTime = performance.now();
        
        // Call WASM function
        const result = hyperionModule.ccall(
            'hyperion_wasm_generate_text',
            'number',
            ['string', 'number', 'number', 'number'],
            [
                options.prompt || '',
                options.maxTokens || 50,
                Math.round((options.temperature || 0.7) * 100),
                options.topK || 40
            ]
        );
        
        const endTime = performance.now();
        const duration = endTime - startTime;
        
        // Convert result to string
        const output = hyperionModule.UTF8ToString(result);
        
        postMessage({
            task: 'generate',
            result: {
                text: output,
                stats: {
                    duration: duration,
                    tokensPerSecond: (options.maxTokens || 50) / (duration / 1000)
                },
                options: options
            }
        });
        
    } catch (error) {
        postMessage({
            task: 'generate',
            error: error.message
        });
    }
}

// Batch processing for multiple requests
function batchGenerate(requests) {
    if (!initialized || !hyperionModule) {
        postMessage({
            task: 'batch',
            error: 'Worker not initialized'
        });
        return;
    }
    
    const results = [];
    const startTime = performance.now();
    
    try {
        for (let i = 0; i < requests.length; i++) {
            const request = requests[i];
            
            const result = hyperionModule.ccall(
                'hyperion_wasm_generate_text',
                'number',
                ['string', 'number', 'number', 'number'],
                [
                    request.prompt || '',
                    request.maxTokens || 25,
                    Math.round((request.temperature || 0.7) * 100),
                    request.topK || 40
                ]
            );
            
            const output = hyperionModule.UTF8ToString(result);
            results.push({
                id: request.id,
                text: output,
                prompt: request.prompt
            });
        }
        
        const endTime = performance.now();
        const totalDuration = endTime - startTime;
        
        postMessage({
            task: 'batch',
            result: {
                results: results,
                stats: {
                    totalDuration: totalDuration,
                    averageDuration: totalDuration / requests.length,
                    requestCount: requests.length
                }
            }
        });
        
    } catch (error) {
        postMessage({
            task: 'batch',
            error: error.message
        });
    }
}

// Memory optimization
function optimizeMemory() {
    if (hyperionModule && hyperionModule._wasm_force_gc) {
        try {
            hyperionModule._wasm_force_gc();
            postMessage({
                task: 'optimize',
                result: { message: 'Memory optimization completed' }
            });
        } catch (error) {
            postMessage({
                task: 'optimize',
                error: error.message
            });
        }
    }
}

// Cleanup resources
function cleanup() {
    if (initialized && hyperionModule) {
        try {
            hyperionModule.ccall('hyperion_wasm_cleanup', null, [], []);
            initialized = false;
            hyperionModule = null;
            
            postMessage({
                task: 'cleanup',
                result: { message: 'Worker cleanup completed' }
            });
        } catch (error) {
            postMessage({
                task: 'cleanup',
                error: error.message
            });
        }
    }
}

// Performance monitoring
function getPerformanceStats() {
    const stats = {
        workerUptime: performance.now(),
        initialized: initialized,
        memoryUsage: 'unavailable' // WASM memory info not easily accessible in worker
    };
    
    postMessage({
        task: 'stats',
        result: stats
    });
}

// Error handler
self.onerror = function(error) {
    postMessage({
        task: 'error',
        error: {
            message: error.message,
            filename: error.filename,
            lineno: error.lineno,
            colno: error.colno
        }
    });
};

// Worker ready signal
postMessage({
    task: 'ready',
    result: { message: 'Hyperion worker ready' }
});