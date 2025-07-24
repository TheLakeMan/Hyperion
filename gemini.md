# Gemini Agent - Hyperion Project Workflow

This document outlines my workflow and the conventions we'll use for the Hyperion project. It serves as our shared context to ensure we work together effectively.

## Project Context
- **Project Name:** Hyperion
- **Primary Language:** C
- **Build System:** CMake

## My Workflow

I approach software engineering tasks using a consistent, four-step process:

1.  **Understand:** I will always start by analyzing the relevant parts of the codebase to understand the context of your request. I'll use tools like `glob` to find files and `read_file` to understand their content.
2.  **Plan:** Before making any changes, I will present a clear, concise plan outlining the steps I intend to take.
3.  **Implement:** Once you approve the plan, I will use the available tools (`write_file`, `replace`, etc.) to execute it. I will follow the existing coding conventions and style.
4.  **Verify:** After implementing changes, I will attempt to verify them by building the project and running tests. This helps ensure that my changes are correct and haven't introduced any regressions.

## Project-Specific Commands

Based on the project structure, I will use the following commands. Please let me know if these are incorrect or if you have a preferred workflow.

**To Build:**
I will use CMake to build the project. I'll create a `build` directory if it doesn't exist.
```bash
if not exist build mkdir build
cd build
cmake ..
cmake --build .
```

**To Run Tests:**
I see several test scripts (e.g., `run_priority_tests.bat`). I will use these when appropriate. For running the full test suite, I will use `ctest` from the build directory.
```bash
cd build
ctest
```

---
We can update this file at any time to refine our process.
