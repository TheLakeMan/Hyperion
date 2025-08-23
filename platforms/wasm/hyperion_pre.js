/**
 * Hyperion WASM Pre-initialization JavaScript
 * 
 * This file is executed before the WASM module is loaded.
 * Sets up the environment and utilities for the Hyperion AI framework.
 */

// Global Hyperion namespace
window.Hyperion = window.Hyperion || {};

// Performance monitoring
Hyperion.performance = {
    startTime: null,
    highMemoryPressure: false,
    
    start: function() {
        this.startTime = performance.now();
    },
    
    end: function(tokenCount) {
        if (this.startTime) {
            const duration = performance.now() - this.startTime;
            const tokensPerSecond = tokenCount / (duration / 1000);
            console.log(`Generation: ${duration.toFixed(2)}ms, ${tokensPerSecond.toFixed(2)} tokens/sec`);
            return { duration, tokensPerSecond };
        }
        return null;
    },
    
    checkMemoryPressure: function() {
        if (performance.memory) {
            const used = performance.memory.usedJSHeapSize;
            const limit = performance.memory.jsHeapSizeLimit;
            this.highMemoryPressure = (used / limit) > 0.8;
            return this.highMemoryPressure;
        }
        return false;
    }
};

// Device detection
Hyperion.device = {
    detect: function() {
        const ua = navigator.userAgent;
        const info = {
            isMobile: /Android|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(ua),
            isLowEnd: false,
            cores: navigator.hardwareConcurrency || 4,
            memory: navigator.deviceMemory || 4
        };
        
        // Estimate if device is low-end
        info.isLowEnd = info.memory < 4 || info.cores < 4;
        
        return info;
    }
};

// Error handling
Hyperion.error = {
    handle: function(error, context) {
        console.error(`Hyperion Error [${context}]:`, error);
        
        // Show user-friendly error
        if (window.showHyperionError) {
            window.showHyperionError(`AI processing error: ${error.message || error}`);
        }
    }
};

// Utility functions
Hyperion.utils = {
    // Convert string to UTF-8 bytes for WASM
    stringToUTF8: function(str) {
        return new TextEncoder().encode(str);
    },
    
    // Convert UTF-8 bytes from WASM to string
    UTF8ToString: function(bytes) {
        return new TextDecoder().decode(bytes);
    },
    
    // Format bytes to human readable
    formatBytes: function(bytes) {
        if (bytes === 0) return '0 B';
        const k = 1024;
        const sizes = ['B', 'KB', 'MB', 'GB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }
};

// Browser compatibility checks
Hyperion.compatibility = {
    check: function() {
        const issues = [];
        
        if (!window.WebAssembly) {
            issues.push('WebAssembly not supported');
        }
        
        if (!window.Worker) {
            issues.push('Web Workers not supported');
        }
        
        if (!window.fetch) {
            issues.push('Fetch API not supported');
        }
        
        return {
            compatible: issues.length === 0,
            issues: issues
        };
    }
};

console.log('Hyperion WASM pre-initialization complete');