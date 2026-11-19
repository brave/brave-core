---
name: brave/reset-test-only-globals-in-teardown
description:
  A test-only global that a fixture's SetUp() changes is restored in TearDown(),
  so its state does not leak into later tests.
rubric: testing
severity: blocking
---

**Rule.** When a fixture changes a test-only global in `SetUp()` (such as
`g_disable_auto_start = true`), it MUST restore the global in `TearDown()`. A
fixture member that restores it on destruction, such as a `base::AutoReset`, has
the same effect. Flag a test-only global the change sets with nothing to reset
it.

**Why.** Tests of a binary share a process, so a global left changed carries
into every test that runs after it: those tests then pass or fail depending on
the order they run in.

## Examples

Bad:

```cpp
void SetUp() override { g_disable_auto_start = true; }
```

Good:

```cpp
void SetUp() override { g_disable_auto_start = true; }
void TearDown() override { g_disable_auto_start = false; }
```
