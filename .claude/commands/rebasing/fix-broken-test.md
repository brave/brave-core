---
name: fix-broken-test
description:
  Diagnose and fix a broken Brave unit or browser test with minimal,
  review-ready changes
---

Goal:

- Identify why a Brave unit test or browser test is failing.
- Determine whether the failure is caused by Brave changes or upstream Chromium
  changes.
- Implement the smallest possible clean fix.
- Verify by rebuilding and rerunning the test.
- Produce a clear, descriptive commit message referencing root cause and
  upstream changes (if applicable).

Inputs:

- Test name or test suite

Phases:

1. Build and reproduce failure

- Build Brave and the Brave unit tests or browser tests (whichever is
  applicable)
- Reuse existing out directories when valid; avoid full clean builds unless
  necessary
- Build the relevant test target (e.g., brave_unit_tests, brave_browser_tests)
- Run the specified test using --gtest_filter with verbose output and stack
  traces enabled
- Capture:
  - Full stack trace
  - Assertion failures
  - Logs and stderr output
- If test is flaky, run multiple times to confirm reproducibility

2. Analyze failure

- Identify:
  - Exact failing assertion or crash site
  - File and line number of failure
  - Call stack leading to failure
- Classify failure type:
  - Assertion mismatch
  - Null/invalid pointer
  - API mismatch
  - Behavior change
  - Timing / async issue
- Determine the _first point of divergence_ from expected behavior

3. Check recent Brave changes

- Inspect recent commits (last ~1-2 weeks) affecting:
  - The failing test
  - Related components/files in the stack trace
- Use `git blame` on failing lines
- Identify any Brave-specific patches that could explain:
  - API changes
  - Behavior overrides
  - Feature flags or guards
- If a likely culprit is found:
  - Validate by reviewing diff and intent

4. Check recent upstream Chromium changes

- Use git log on corresponding Chromium paths
- Identify corresponding upstream files/modules
- Review upstream commits from the past several weeks affecting:
  - Same files
  - APIs used in the failing stack
- Look for:
  - Signature changes
  - Behavior changes
  - Test expectation updates
- Compare Brave code vs upstream to detect divergence

5. Determine root cause

- Decide whether the failure is due to:
  - Brave regression
  - Upstream API/behavior change
  - Test becoming outdated
  - Integration mismatch
- Clearly identify:
  - What changed
  - Why the test now fails

6. Implement minimal fix

- Apply the smallest possible change that:
  - Fixes the root cause
  - Preserves intended behavior
- Prefer:
  - Updating Brave adaptation layer over patching upstream code
  - Adjusting test expectations only if behavior change is correct
- Avoid:
  - Broad refactors
  - Unnecessary formatting or unrelated edits
- Keep diff minimal and localized
- Prefer aligning Brave behavior with upstream unless Brave intentionally
  diverges

7. Validate fix

- Rebuild affected targets
- Rerun the failing test
- Confirm:
  - Test passes consistently
  - No new failures introduced in related tests or same test binary

8. Commit changes

- Create a single clean commit
- Commit message format:

  <component>: Fix failing <test name>

  Problem:

  - Brief description of failure and symptom

  Root cause:

  - Explanation of what caused the issue
  - Reference Brave or upstream change

  Fix:

  - Description of minimal change made

  Notes:

  - Mention if behavior aligns with upstream
  - Include upstream commit hash, CL, or bug link if identifiable

- Ensure commit is review-ready:
  - No debug code
  - No unrelated changes
  - Clear and concise message

Heuristics:

- If failure is due to API mismatch -> likely upstream change
- If failure involves brave/ or chromium_src/ code -> likely Brave regression
- If test expectations differ from upstream -> check upstream test updates first
- If null/segfault -> prioritize ownership and lifetime changes in recent
  commits

Constraints:

- Do not modify upstream files unless absolutely necessary (prefer chromium_src
  overrides or minimal patching if required)
- Preserve existing architecture and intent
- Do not guess root cause; if unclear, gather more evidence before modifying
  code
- Avoid speculative fixes; ground decisions in evidence from stack trace and
  commits
- Keep changes as small and surgical as possible

Anti-patterns to avoid:

- Blindly updating test expectations without confirming correct behavior
- Adding sleeps/timeouts to fix timing issues without root cause
- Overriding upstream behavior unnecessarily
- Large refactors unrelated to the failure
