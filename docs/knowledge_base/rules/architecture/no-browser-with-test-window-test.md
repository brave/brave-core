---
name: brave/no-browser-with-test-window-test
description:
  'Do not write tests on BrowserWithTestWindowTest, which forces test-only if
  (!browser_view) checks into production code; use browser tests or unit tests
  with injected dependencies.'
rubric: testing
severity: should_fix
---

**Rule.** Do not base new tests on `BrowserWithTestWindowTest`, and do not add
`if (!browser_view)`-style checks to production code to make such tests pass.
Write a browser test, or a unit test that injects the feature's dependencies
(for tab and window features, see
`brave/override-user-data-factory-in-integration-tests`).

**Why.** `BrowserWithTestWindowTest` builds a partial browser without a real
window, so the code under test needs test-only null checks, and the test
exercises a configuration production never has. Chromium discourages it.

## Examples

Bad:

```cpp
class FooControllerTest : public BrowserWithTestWindowTest {};

// In production code:
if (!browser_view) {
  return;  // only happens in BrowserWithTestWindowTest
}
```

Good:

```cpp
class FooControllerBrowserTest : public InProcessBrowserTest {};
```
