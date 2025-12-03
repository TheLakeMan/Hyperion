# IDE Configuration Samples

This folder contains lightweight configuration snippets for common IDE workflows built around the CMake toolchain used by Hyperion.

## Visual Studio Code

The `vscode/` directory provides ready-to-use task and launch definitions that assume a standard out-of-source build in `./build`.

- `tasks.json` defines tasks for configuring the project, building targets, running the test suite via `ctest`, and cleaning artifacts.
- `launch.json` includes debug profiles for the main `hyperion` binary (with the interactive shell as the default argument) and the `hyperion_tests` binary.

Copy these files into `.vscode/` at the repository root or point your IDE to read them directly from `tools/ide/vscode/`.
