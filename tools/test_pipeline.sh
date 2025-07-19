#!/bin/bash
# Hyperion Pipeline Test Script
# This script demonstrates the full Hyperion pipeline with the sample data

echo "=== Hyperion Pipeline Test ==="
echo

# Step 1: Load the model and tokenizer
echo "Step 1: Load the model and tokenizer..."
cd ..
./build/hyperion model load data/model_config.json data/tiny_vocab.tok

# Step 2: Test tokenizer with sample text
echo
echo "Step 2: Testing tokenizer with sample text..."
./build/hyperion tokenize "Hyperion framework supports both local and remote execution with 4-bit quantization."

# Step 3: Generate text with local-only mode
echo
echo "Step 3: Generating text with local model..."
./build/hyperion config verbose 1
./build/hyperion generate "Hyperion is an ultra-lightweight AI framework that" 30 0.7 top_k

# Step 4: Test MCP connection (this is a simulated connection)
echo
echo "Step 4: Testing MCP connection..."
./build/hyperion mcp connect mock://localhost:8080
./build/hyperion mcp status

# Step 5: Enable hybrid mode and compare with local
echo
echo "Step 5: Testing hybrid generation mode..."
./build/hyperion hybrid on
./build/hyperion hybrid status
./build/hyperion generate "The key features of Hyperion include 4-bit quantization and" 20 0.8 top_p

# Step 6: Force local mode
echo
echo "Step 6: Forcing local mode for comparison..."
./build/hyperion hybrid force-local
./build/hyperion generate "Hyperion's tokenizer supports basic BPE operations and can" 20 0.8 top_p

# Step 7: Force remote mode
echo
echo "Step 7: Forcing remote mode for comparison..."
./build/hyperion hybrid force-remote
./build/hyperion generate "Hybrid execution mode allows Hyperion to balance performance with" 20 0.8 top_p

# Step 8: Disconnect from MCP server
echo
echo "Step 8: Disconnecting from MCP server..."
./build/hyperion mcp disconnect
./build/hyperion mcp status

# Step A: Run comprehensive tests
echo
echo "Step A: Running comprehensive tests..."
cd build
ctest -V

# Done
echo
echo "=== Test Sequence Complete ==="
