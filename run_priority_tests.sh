#!/bin/bash
# Hyperion simplified test runner
set -euo pipefail

echo "===== Hyperion Priority Test Execution ====="

mkdir -p build
cd build
cmake ..
cmake --build .
ctest --output-on-failure
