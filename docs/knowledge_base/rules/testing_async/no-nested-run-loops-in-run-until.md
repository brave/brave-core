---
name: brave/no-nested-run-loops-in-run-until
description:
  The condition passed to base::test::RunUntil() must not run its own run loop,
  as EvalJs(), ExecJs() and waiting IPCs do.
rubric: testing
severity: blocking
---

**Rule.** The predicate given to `base::test::RunUntil()` MUST NOT call anything
that spins a run loop: `content::EvalJs()`, `content::ExecJs()`, or an IPC that
waits for its response. Keep it to direct C++ state checks and simple getters.
To wait for a page condition, poll in JavaScript or use a `MutationObserver`,
and evaluate the script once outside `RunUntil()`.

**Why.** `RunUntil()` already runs a loop while it polls the predicate. A
predicate that starts a nested loop runs unrelated tasks re-entrantly in the
middle of the check, which makes the test hang or flake.

## Examples

Bad:

```cpp
ASSERT_TRUE(base::test::RunUntil([&]() {
  return content::EvalJs(web_contents(), "!!document.getElementById('x')")
      .ExtractBool();
}));
```

Good:

```cpp
ASSERT_TRUE(base::test::RunUntil(
    [&]() { return tab_helper()->speedreader_bubble_view() != nullptr; }));
```
