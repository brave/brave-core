---
name: fix-wip
description: Fix a single WIP commit according to project rules
---

Goal:

- Complete a WIP commit so it is review-ready
- Ensure minimal diffs and preserve intent
- Ensure that code compiles and tests pass

Input:

- Commit (SHA or reference)

Phases:

1. Inspect commit

- Identify WIP indicators in commit message (e.g., "WIP", "tmp", "temp",
  "debug", "DO NOT SUBMIT")
- Inspect commit message and commit
- Inspect upstream commit message and upstream commit only if explicitly
  provided
- If no WIP indicators are present and no incomplete work is detected and the
  commit appears complete and compliant:
  - Do not modify it
  - Report as "skipped"

2. Identify and make change

- If an upstream commit is explicitly provided:
  - Confirm whether it caused the issue
- Otherwise:
  - Do not infer or invent an upstream cause
- Only treat an upstream commit as identifiable if directly referenced in the
  commit or provided context
- If upstream commit is relevant:
  - Align behavior with upstream intent
  - Do not diverge unless necessary
- Apply the smallest possible diff that fully resolves the issue
- Do not produce partial fixes that leave the code in a broken or inconsistent
  state
- Do not modify code outside the direct scope of the problem
- Avoid renaming, reformatting, or moving code unless required
- Preserve functionality
- Prefer adapting existing code over introducing new patterns
- Do not refactor unless required to fix the issue
- Do not create new files unless strictly required to fix the issue

3. Build and test

- Ensure that project builds successfully using the project's standard build
  commands
- Ensure that all tests pass using the project's standard test commands
- Run build before tests
- If build fails:
  - Attempt minimal fixes
  - If minimal fixes do not resolve the issue, report failure
  - Do not proceed to tests if build fails
- Only proceed to commit rewrite if build and tests both succeed

4. Rewrite commit message

- Remove WIP marker from commit subject
- Ensure message follows conventional commit / project style
- Use imperative voice (e.g., "Fix crash in X")
- Be concise but descriptive
- Preserve author and timestamp
- Preserve the original intent of the commit message while improving clarity
- Add "Note: AI-assisted: requires thorough review" to end of commit message

5. Output

- Status: success | skipped | failed
- Change type: code fix | test fix | build fix | no-op
- Approximate size of change (e.g., lines added/removed)
- Confidence: high | medium | low
- New commit SHA (if modified)
- Files changed (in the final amended commit)
- Summary of changes made
- One-line commit subject
- 2-3 sentence commit description
- Reference relevant upstream commits if applicable

Assumptions:

- The repository is in a clean working state
- Required build tools and dependencies are available
- Do not attempt to fix unrelated environment or infrastructure issues

Commit constraints:

- Modify the existing commit in-place using --fixup=amend, do not create
  additional commits unless absolutely necessary
- Do not reorder commits
- Do not squash with other commits
- Do not perform an interactive rebase

Execution:

- Indicate which commit is being processed
- Print progress at each phase
- Do not repeatedly attempt fixes beyond a small number of iterations
- Stop execution immediately after a failure is reported
- Report success, skip, or failure explicitly

Rules:

- Do not modify unrelated files
- Do not introduce new dependencies
- Do not modify tests unless they are clearly incorrect or outdated
- Keep diffs minimal and reviewable
- Never lose test coverage or public API intent
- Prefer fixing code over relaxing tests

Safety:

- Do not perform push, force-push, or any remote operations
- Do not modify branches outside the specified commit

Failure handling:

- If the issue cannot be confidently resolved:
  - Do not modify the commit
  - Output a clear explanation of the blocker
