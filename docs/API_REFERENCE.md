# Hyperion API Reference

This reference summarizes the public interfaces exposed by the current C headers under `core/` and `interface/`. Use these APIs to initialize runtime services, manage configuration, and drive the command-line interface.

## Core Subsystems

### Activation (`core/activation.h`)
- `void hyperionInitActivationTables(void);`
  - Prepare activation lookup tables before model execution.
- `void hyperionCleanupActivationTables(void);`
  - Release activation resources during shutdown.

### Configuration (`core/config.h`)
- `int hyperionConfigInit(void);`
  - Initialize configuration storage; returns `0` on success.
- `void hyperionConfigSetDefaults(void);`
  - Populate default runtime and generation settings.
- `void hyperionConfigCleanup(void);`
  - Clean up configuration resources.

### I/O System (`core/io.h`)
- `int hyperionIOInit(void);`
  - Initialize platform I/O abstractions; returns `0` on success.
- `void hyperionIOCleanup(void);`
  - Tear down the I/O layer and release platform handles.

### Memory Tracking (`core/memory.h`)
- `int hyperionMemTrackInit(void);`
  - Start memory tracking; returns `0` on success.
- `void hyperionMemTrackCleanup(void);`
  - Stop memory tracking and release internal bookkeeping data.
- `int hyperionMemTrackDumpLeaks(void);`
  - Report detected memory leaks; returns the number of leaked allocations.

## CLI Context (`interface/cli.h`)

### Data Structures
- `HyperionGenerationParams`
  - `int maxTokens` — maximum tokens to generate.
  - `int samplingMethod` — sampling strategy identifier (e.g., `TINYAI_SAMPLING_TOP_P`).
  - `float temperature` — sampling temperature.
  - `int topK` — `top-k` cutoff for sampling.
  - `float topP` — nucleus sampling threshold.
  - `int seed` — RNG seed (`0` selects a random seed).
- `HyperionCLIContext`
  - `HyperionGenerationParams params` — generation defaults.
  - `bool interactive` — whether to run in interactive shell mode.
  - `bool verbose` — enable verbose logging.

### CLI Lifecycle
- `int hyperionCLIInit(HyperionCLIContext *context);`
  - Initialize CLI state; returns `0` when the context is valid.
- `int hyperionCLIParseArgs(HyperionCLIContext *context, int argc, char **argv);`
  - Parse CLI flags such as `--interactive` (`-i`) and `--verbose` (`-v`).
- `int hyperionCLIRun(HyperionCLIContext *context, int argc, char **argv);`
  - Execute the CLI session in interactive or batch mode based on parsed arguments.
- `void hyperionCLICleanup(HyperionCLIContext *context);`
  - Release resources associated with the CLI context.

## Integration Walkthroughs

### Running the CLI

The `hyperion` binary initializes the core subsystems, configures default generation parameters, and runs the CLI:

```bash
# Interactive shell with verbose startup logs
./hyperion --interactive --verbose

# Batch invocation using default parameters
./hyperion
```

The `--interactive` flag triggers the banner and interactive prompt, while `--verbose` emits startup details such as the configured `max_tokens` and `temperature`.

### Embedding the C API

Use the core and CLI helpers to bootstrap Hyperion inside your own C entry point:

```c
#include "core/memory.h"
#include "core/io.h"
#include "core/config.h"
#include "core/activation.h"
#include "interface/cli.h"

int main(int argc, char **argv) {
    if (hyperionIOInit() != 0 || hyperionMemTrackInit() != 0 || hyperionConfigInit() != 0) {
        return 1; // Initialization failed
    }

    hyperionConfigSetDefaults();
    hyperionInitActivationTables();

    HyperionCLIContext context = {0};
    context.params.maxTokens = 100;
    context.params.samplingMethod = TINYAI_SAMPLING_TOP_P;
    context.params.temperature = 0.7f;
    context.params.topK = 40;
    context.params.topP = 0.9f;
    context.params.seed = 0; // random seed

    if (hyperionCLIInit(&context) == 0 &&
        hyperionCLIParseArgs(&context, argc, argv) == 0) {
        hyperionCLIRun(&context, argc, argv);
    }

    hyperionCLICleanup(&context);
    hyperionCleanupActivationTables();
    hyperionConfigCleanup();
    hyperionMemTrackCleanup();
    hyperionIOCleanup();
    return 0;
}
```

This sequence mirrors the default `main` routine: initialize subsystems, set defaults, configure the CLI context, parse arguments, run the CLI, and clean up.
