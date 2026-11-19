---
name: brave/follow-chromium-patterns-for-async-test-fixes
description:
  A fix for an asynchronous or timing problem in a test uses the established
  Chromium pattern for that situation rather than a novel mechanism.
rubric: testing
severity: should_fix
---

**Rule.** When a change fixes flakiness, timing or another asynchronous problem
in a test, its waiting mechanism follows a pattern Chromium's own tests use for
the same requirement, and prefers a deterministic, event-driven one (an
observer, a `MutationObserver` in the page, `base::test::TestFuture`) to an
improvised one. Flag a novel wait, retry loop or delay where Chromium tests
already solve the same problem in an established way.

**Why.** Chromium's test patterns for asynchronous work have been hardened
against the timing variations of its bots; an improvised fix tends to move the
race rather than remove it, and is harder for the next reader to recognise.

## Examples

Bad:

```cpp
// Poll the DOM from C++ until the text changes.
for (int i = 0; i < 50; ++i) {
  if (content::EvalJs(web_contents, "getText()") == "done") {
    break;
  }
  NonBlockingDelay(base::Milliseconds(100));
}
```

Good:

```cpp
// Pattern from service_worker_internals_ui_browsertest.cc: a
// MutationObserver resolves once the DOM holds the expected value.
EXPECT_EQ("done", content::EvalJs(web_contents, kWaitForTextScript));
```
