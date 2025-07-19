@echo off
REM Hyperion Pipeline Test Script
REM This script demonstrates the full Hyperion pipeline with the sample data

echo === Hyperion Pipeline Test ===
echo.

REM Step 1: Load the model and tokenizer
echo Step 1: Load the model and tokenizer...
cd ..
build\Debug\hyperion.exe model load data\model_config.json data\tiny_vocab.tok

REM Step 2: Test tokenizer with sample text
echo.
echo Step 2: Testing tokenizer with sample text...
build\Debug\hyperion.exe tokenize "Hyperion framework supports both local and remote execution with 4-bit quantization."

REM Step 3: Generate text with local-only mode
echo.
echo Step 3: Generating text with local model...
build\Debug\hyperion.exe config verbose 1
build\Debug\hyperion.exe generate "Hyperion is an ultra-lightweight AI framework that" 30 0.7 top_k

REM Step 4: Test MCP connection (this is a simulated connection)
echo.
echo Step 4: Testing MCP connection...
build\Debug\hyperion.exe mcp connect mock://localhost:8080
build\Debug\hyperion.exe mcp status

REM Step 5: Enable hybrid mode and compare with local
echo.
echo Step 5: Testing hybrid generation mode...
build\Debug\hyperion.exe hybrid on
build\Debug\hyperion.exe hybrid status
build\Debug\hyperion.exe generate "The key features of Hyperion include 4-bit quantization and" 20 0.8 top_p

REM Step 6: Force local mode
echo.
echo Step 6: Forcing local mode for comparison...
build\Debug\hyperion.exe hybrid force-local
build\Debug\hyperion.exe generate "Hyperion's tokenizer supports basic BPE operations and can" 20 0.8 top_p

REM Step 7: Force remote mode
echo.
echo Step 7: Forcing remote mode for comparison...
build\Debug\hyperion.exe hybrid force-remote
build\Debug\hyperion.exe generate "Hybrid execution mode allows Hyperion to balance performance with" 20 0.8 top_p

REM Step 8: Disconnect from MCP server
echo.
echo Step 8: Disconnecting from MCP server...
build\Debug\hyperion.exe mcp disconnect
build\Debug\hyperion.exe mcp status

REM Step A: Run comprehensive tests
echo.
echo Step A: Running comprehensive tests...
cd build
ctest -V

REM Done
echo.
echo === Test Sequence Complete ===
