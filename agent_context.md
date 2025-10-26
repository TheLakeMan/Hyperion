# Hyperion Agent Context

## Project Overview
- **Hyperion** is an ultra-lightweight AI framework written in C that focuses on extreme memory efficiency through 4-bit quantization, sparse matrix operations, and SIMD-accelerated kernels.
- Supports hybrid execution via the Model Context Protocol (MCP), enabling seamless switching between local and remote inference while tracking performance.
- Provides multimodal capabilities (text, image, audio) with tooling that includes a CLI, C API, and web server components.

## Repository Structure Highlights
- `core/`: Picol interpreter extensions, configuration, I/O, and memory subsystems.
- `models/`: Implementations and utilities for text, audio, and model format handling.
- `utils/`: Model loading, quantization, SIMD operations, and supporting helpers.
- `tests/`: Unit and integration tests covering I/O, memory, and model behavior.
- `interface/`, `tools/`, `scripts/`, `web_ui/`: Platform interfaces, developer tooling, automation scripts, and web assets.
- Additional documentation: see `ARCHITECTURE.md`, `DEVELOPMENT.md`, `HYBRID_CAPABILITIES.md`, and `TEST_PLAN.md` for deep dives.

## Build & Test Workflows
- Standard build: `mkdir build && cd build && cmake .. && cmake --build . --config Release`.
- Execute tests: from the `build` directory run `ctest -C Release` after building.
- Quick scripts: `run_priority_tests.sh` and equivalents provide targeted test execution.

## Recent Repository History
- Latest commits (`git log --oneline -5`):
  - `793ca62` Clean up repository: Remove Qoder IDE-specific files (duplicate hash retained upstream).
  - `d9387f5` COMPLETE Phase 4: Platform Expansion - Add ALL missing files.
  - `286abd9` Add missing tensor.h header file.
  - `3a78325` Fix syntax errors and file formatting issues.
  - `86147ef` Phase 5 preparation: Add WASM platform implementation and save previous chat history.

## Current Working Tree Status
- Branch `main` (ahead 1, behind 4 relative to `origin/main`).
- Modified tracked files:
  - `CMakeLists.txt`
  - `core/config.c`, `core/enhanced_errors.c`, `core/enhanced_errors.h`, `core/io.c`, `core/memory.c`, `core/memory.h`, `core/picol.c`, `core/picol.h`
  - `models/audio/audio_utils.c`, `models/text/generate.c`
  - `tests/test_io.c`, `tests/test_main.c`, `tests/test_memory.c`
  - `utils/model_loader.c`, `utils/quantize.c`, `utils/quantize.h`, `utils/simd_ops.c`
- Untracked files:
  - `agent.md`
  - `agent_context.md`
  - `models/model_format.c`, `models/model_format.h`
  - `tests/test_framework.c`, `tests/test_framework.h`, `tests/test_model_format.c`

## Recent Test Execution
- `cmake --build /home/thelakeman/Hyperion/build`
- `ctest --test-dir /home/thelakeman/Hyperion/build --output-on-failure`

## Recent Fixes
- Corrected text sampling logic (`sampleTopK`/`sampleTopP`) to remove invalid references and ensure deterministic fallbacks when probability mass is zero.
- Extracted sampling utilities into `models/text/sampling.c` and added dedicated unit coverage in `tests/test_sampling.c` for top-k, top-p, and greedy pathways.

## Usage Notes for Droid Agents
- Reference `agent.md` for operational guidelines, then consult this document for project scope.
- Before implementation tasks, review relevant documentation in the repository and ensure builds/tests pass for impacted areas.
- Maintain up-to-date knowledge of current working-tree changes to avoid conflicts or regressions.
