# Assignment

Implement a control-flow graph (CFG) and static single assignment (SSA) form for
one C# method at a time. The input is a method signature and block body, without
a surrounding class. Parsing a construct does not imply CFG/SSA support.

## LLM assistance

LLM assistance is permitted for AST parsing, visualization, UI, documentation,
examples, and tests. **Strictly do not write or modify CFG or SSA implementation
code**, including their algorithms, lowering, and semantic data structures.
`src/cfg.cpp` and `include/lab/cfg.hpp`, and any future SSA implementation, are
student-owned. Report problems in those implementations without fixing them.
The user authorized the initial `SSA.hpp` / `SSA.cpp` error-returning scaffold
and its protocol/CLI wiring only; this does not authorize SSA algorithm changes.
Tests may be updated to reflect observed output; do not claim this proves CFG
or SSA semantic correctness.

## Code style

- Follow the surrounding style; keep changes small and focused.
- C++20: four-space indentation, snake_case functions, headers in `include/lab`.
- TypeScript/JavaScript: two-space indentation, single quotes, semicolons.
- Keep graph presentation and HTTP/process handling separate from analysis.
- Preserve UTF-8 source text and byte ranges; convert to UTF-16 only for Monaco.
- Use Russian UI labels, matching the existing viewer. Add no unnecessary dependencies.

## Checks

Build with `cmake --build --preset windows-debug` and run
`ctest --preset windows-debug` (use `linux-debug` on Linux).
In `viewer`, run `npm run build`, `npm test`, and `npm run test:e2e` for UI changes.
Use `npm.cmd` in Windows PowerShell. Tests require the compiled analyzer;
Playwright also needs an installed browser (`PLAYWRIGHT_CHANNEL=chrome` can use Chrome).
