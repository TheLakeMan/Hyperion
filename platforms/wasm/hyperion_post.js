/**
 * Hyperion WASM Post-initialization JavaScript
 * 
 * This file is executed after the WASM module is loaded.
 * Provides the complete JavaScript API for the Hyperion AI framework.
 */

// Initialize the Hyperion API after WASM module is ready
Module.onRuntimeInitialized = function() {
    console.log('Hyperion WASM runtime initialized');
    
    // Wrap WASM functions for easier JavaScript usage
    Hyperion.wasm = {
        // Initialize Hyperion
        init: Module.cwrap('hyperion_wasm_init', 'number', []),
        
        // Generate text
        generateText: Module.cwrap('hyperion_wasm_generate_text', 'number', 
            ['string', 'number', 'number', 'number']),
        
        // Cleanup
        cleanup: Module.cwrap('hyperion_wasm_cleanup', null, [])
    };
    
    // High-level API
    Hyperion.api = {
        initialized: false,
        
        // Initialize the AI framework
        async init() {
            try {
                const result = Hyperion.wasm.init();
                if (result === 0) {
                    this.initialized = true;
                    console.log('✅ Hyperion initialized successfully');
                    return true;
                } else {
                    throw new Error(`Initialization failed with code: ${result}`);
                }
            } catch (error) {
                Hyperion.error.handle(error, 'initialization');
                return false;
            }
        },
        
        // Generate text with options
        async generate(prompt, options = {}) {
            if (!this.initialized) {
                throw new Error('Hyperion not initialized. Call init() first.');
            }
            
            // Default options
            const opts = {
                maxTokens: options.maxTokens || 100,
                temperature: options.temperature || 0.7,
                topK: options.topK || 40,
                ...options
            };
            
            // Check device capabilities and adjust
            const device = Hyperion.device.detect();
            if (device.isLowEnd) {
                opts.maxTokens = Math.min(opts.maxTokens, 50);
                console.log('🔋 Low-end device detected, reducing token limit');
            }
            
            // Check memory pressure
            if (Hyperion.performance.checkMemoryPressure()) {
                opts.maxTokens = Math.min(opts.maxTokens, 25);
                console.log('⚠️ High memory pressure, further reducing tokens');
            }
            
            try {
                Hyperion.performance.start();
                
                const result = Hyperion.wasm.generateText(
                    prompt,
                    opts.maxTokens,
                    Math.round(opts.temperature * 100),
                    opts.topK
                );
                
                const stats = Hyperion.performance.end(opts.maxTokens);
                
                // Convert result pointer to string
                const output = Module.UTF8ToString(result);
                
                return {
                    text: output,
                    stats: stats,
                    options: opts
                };
            } catch (error) {
                Hyperion.error.handle(error, 'generation');
                throw error;
            }
        },
        
        // Cleanup resources
        cleanup() {
            if (this.initialized) {
                Hyperion.wasm.cleanup();
                this.initialized = false;
                console.log('🧹 Hyperion cleanup completed');
            }
        }
    };
    
    // Worker support for background processing
    Hyperion.worker = {
        instance: null,
        
        // Create a worker for background processing
        create() {
            if (typeof Worker !== 'undefined') {
                this.instance = new Worker('hyperion_worker.js');
                this.instance.onmessage = this.handleMessage;
                this.instance.onerror = this.handleError;
                console.log('👷 Hyperion worker created');
                return true;
            }
            return false;
        },
        
        // Send task to worker
        postTask(task, data) {
            if (this.instance) {
                this.instance.postMessage({ task, data });
            }
        },
        
        // Handle worker messages
        handleMessage(event) {
            const { task, result, error } = event.data;
            
            if (error) {
                Hyperion.error.handle(error, `worker-${task}`);
            } else {
                console.log(`Worker task '${task}' completed:`, result);
            }
        },
        
        // Handle worker errors
        handleError(error) {
            Hyperion.error.handle(error, 'worker');
        },
        
        // Terminate worker
        terminate() {
            if (this.instance) {
                this.instance.terminate();
                this.instance = null;
                console.log('🔚 Hyperion worker terminated');
            }
        }
    };
    
    // Auto-optimization based on device
    Hyperion.optimize = {
        // Automatically optimize settings for current device
        auto() {
            const device = Hyperion.device.detect();
            const compat = Hyperion.compatibility.check();
            
            console.log('🔧 Auto-optimizing for device:', device);
            
            if (!compat.compatible) {
                console.warn('⚠️ Compatibility issues:', compat.issues);
            }
            
            // Create worker if supported and not low-end
            if (!device.isLowEnd && compat.compatible) {
                Hyperion.worker.create();
            }
            
            return {
                workerEnabled: !!Hyperion.worker.instance,
                recommendedMaxTokens: device.isLowEnd ? 25 : 100,
                recommendedTemperature: 0.7,
                device: device
            };
        }
    };
    
    // Initialize automatically
    Hyperion.api.init().then(() => {
        // Auto-optimize
        const optimization = Hyperion.optimize.auto();
        console.log('⚡ Hyperion optimization:', optimization);
        
        // Notify that Hyperion is ready
        window.dispatchEvent(new CustomEvent('hyperionReady', {
            detail: { optimization }
        }));
    });
    
    // Cleanup on page unload
    window.addEventListener('beforeunload', () => {
        Hyperion.worker.terminate();
        Hyperion.api.cleanup();
    });
};

// Export for Node.js environment
if (typeof module !== 'undefined' && module.exports) {
    module.exports = { Hyperion };
}