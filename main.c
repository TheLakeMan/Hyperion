/**
 * Hyperion Main Entry Point
 * 
 * This file contains the main function for the Hyperion command-line application.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/memory.h"
#include "core/io.h"
#include "core/config.h"
#include "core/activation.h"
#include "interface/cli.h"

/* Application version */
#define TINYAI_VERSION "0.1.0"

/**
 * Print application banner
 */
void printBanner() {
    printf("Hyperion v%s - Ultra-Lightweight AI Framework\n", TINYAI_VERSION);
    printf("Memory-efficient 4-bit quantized neural networks\n");
    printf("----------------------------------------------------------------\n");
}

/**
 * Main entry point
 */
int main(int argc, char *argv[]) {
    int result = 0;
    
    /* Initialize subsystems */
    if (hyperionIOInit() != 0) {
        fprintf(stderr, "Failed to initialize I/O system\n");
        return 1;
    }
    
    if (hyperionMemTrackInit() != 0) {
        fprintf(stderr, "Failed to initialize memory tracking\n");
        hyperionIOCleanup();
        return 1;
    }
    
    if (hyperionConfigInit() != 0) {
        fprintf(stderr, "Failed to initialize configuration system\n");
        hyperionMemTrackCleanup();
        hyperionIOCleanup();
        return 1;
    }
    
    /* Set default configuration */
    hyperionConfigSetDefaults();
    
    /* Initialize activation lookup tables */
    hyperionInitActivationTables();
    
    /* Initialize CLI context */
    HyperionCLIContext context;
    memset(&context, 0, sizeof(context));
    
    /* Set default generation parameters */
    context.params.maxTokens = 100;
    context.params.samplingMethod = TINYAI_SAMPLING_TOP_P;
    context.params.temperature = 0.7f;
    context.params.topK = 40;
    context.params.topP = 0.9f;
    context.params.seed = 0;  /* Use random seed */
    
    /* Initialize CLI */
    if (hyperionCLIInit(&context) != 0) {
        fprintf(stderr, "Failed to initialize CLI\n");
        result = 1;
        goto cleanup;
    }
    
    /* Parse command-line arguments */
    if (hyperionCLIParseArgs(&context, argc, argv) != 0) {
        fprintf(stderr, "Failed to parse command-line arguments\n");
        result = 1;
        goto cleanup;
    }
    
    /* Print banner in interactive mode */
    if (context.interactive) {
        printBanner();
    }
    
    /* Run CLI */
    result = hyperionCLIRun(&context, argc, argv);

    if (context.memReport) {
        hyperionMemTrackDumpReport(stdout);
    }

    /* Check for memory leaks in verbose mode */
    if (context.verbose) {
        int leaks = hyperionMemTrackDumpLeaks();
        if (leaks > 0) {
            fprintf(stderr, "Warning: %d memory leaks detected\n", leaks);
        } else {
            printf("No memory leaks detected\n");
        }
    }
    
cleanup:
    /* Clean up CLI */
    hyperionCLICleanup(&context);
    
    /* Clean up activation tables */
    hyperionCleanupActivationTables();
    
    /* Clean up subsystems */
    hyperionConfigCleanup();
    hyperionMemTrackCleanup();
    hyperionIOCleanup();
    
    return result;
}
