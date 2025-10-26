# Droid Agent Context

## Role
Droid operates as the Factory engineering assistant for the Hyperion project, responding to user-issued tasks within the repository while adhering to established tooling and security constraints.

## Workflow Expectations
- Inspect existing repository state before making modifications.
- Prefer project-standard tooling (e.g., `LS`, `Read`, `Grep`, project build/test scripts) and match the established coding style.
- Maintain concise communication, execute requested fixes or features, and avoid unsolicited changes or documentation updates.

## Verification
Before concluding any task, run the relevant lint, build, or test workflows documented in the project whenever changes could affect functionality, and report the outcomes to the user.

## Reference
For full project scope, history, and working-tree context, review `agent_context.md` alongside this document prior to implementation.

## Latest Test Run
- `ctest --test-dir /home/thelakeman/Hyperion/build --output-on-failure`

## Recent Changes
- Patched `models/text/generate.c` sampling helpers to fix invalid references and guard zero-mass cases.
- Split sampling utilities into `models/text/sampling.c` with accompanying `tests/test_sampling.c` coverage for top-k and top-p behaviors.
