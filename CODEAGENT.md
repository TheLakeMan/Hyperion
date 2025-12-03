# Code Agent Best Practices

## Guiding Principles

This document outlines best practices for automated coding agents when implementing changes to the Hyperion project.

### Efficiency Through Completeness

**File Rewrites Over Surgical Edits**

When modifying a file, assess whether surgical edits (targeted replacements) or a complete rewrite would be more efficient:

- **Use complete rewrites when:**
  - The file has multiple interconnected issues
  - Line-ending or formatting problems affect many lines
  - Structural changes span significant portions of the file
  - Context and clarity benefit from a fresh organization
  
- **Use surgical edits when:**
  - Changes are isolated to specific sections
  - The file structure is sound and only specific logic needs updating
  - Minimal changes reduce risk of unintended side effects

**Example:** When `CMakeLists.txt` had merge artifacts (`>`, `+` markers), duplicated lines, and a broken command spread across lines, rewriting the entire file was faster and safer than hunting for each issue.

### Testing Is Non-Negotiable

**After Every Code Change**

1. **Test Immediately**: Don't wait to batch-test changes. Test each modification as soon as it's committed or written.
2. **Validate Syntax**: Run appropriate linters, compilers, or test runners to confirm the code parses and runs.
3. **Don't Move Forward Incomplete**: If a change doesn't pass its test, fix it before proceeding to the next task.
4. **Document the Test**: Include the test command or approach in commit messages or task notes.

**Testing Workflow:**
```bash
# After editing a CMake file:
cmake -B build -S .  # Validate parsing and configuration

# After editing Python:
python -m py_compile script.py  # Check syntax
# or run tests directly

# After code changes:
./build-and-test.sh  # Run full build/test suite
```

### Workflow Discipline

- **Plan First**: Use `manage_todo_list` to outline multi-step work.
- **Mark Progress**: Update task status immediately upon completion—don't batch completions.
- **Verify Before Commit**: Ensure no errors exist locally before pushing.
- **Push Verified Code**: GitHub Actions should only run against working code.

### Communication

When a fix is applied:
- State what was wrong (e.g., "merge artifacts in CMakeLists.txt").
- State what was fixed (e.g., "removed stray markers, repaired broken command").
- Note the test result (e.g., "CMake now parses correctly").
- Offer next steps or ask for confirmation to proceed.

---

## Example: The CMakeLists.txt Fix

**Problem Identified:**  
Workflow logs showed: `CMake Error at CMakeLists.txt:4: Parse error. Expected a command name, got unquoted argument with text ">".`

**Analysis:**  
- Merge left `>` and `+` markers on lines 4–6
- Duplicate `find_package(Python3...)` statements
- Broken `add_custom_target` command split across physical lines

**Solution:**  
Rather than fixing each line individually, the entire file was rewritten, preserving all valid CMake logic while eliminating noise.

**Validation:**  
Pushed to `main`, workflow re-runs should now pass CMake configuration step.

---

## Key Takeaways

1. **Rewrite files when it's simpler**—don't over-engineer surgical edits.
2. **Test after every change**—no exceptions, no batch testing.
3. **Communicate clearly**—what, why, how, and proof it works.
4. **Document decisions**—future maintainers (human or agent) benefit from context.

