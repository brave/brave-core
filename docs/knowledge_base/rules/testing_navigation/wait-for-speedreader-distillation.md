---
name: brave/wait-for-speedreader-distillation
description:
  A Speedreader test waits for distillation to complete, with WaitDistilled(),
  before it interacts with distilled content, because
  NavigateToPageSynchronously() only waits for load stop.
rubric: testing
severity: blocking
---

**Rule.** In tests of distilled (Speedreader) content, ALWAYS wait for
distillation to complete, with `WaitDistilled()`, after navigating and before
interacting with or checking the distilled page.
`NavigateToPageSynchronously()`, like any wait for load stop, returns before
distillation is done.

**Why.** Distillation finishes after the page stops loading, so a test that
touches distilled elements right after navigating races with it and fails
intermittently.

## Examples

Bad:

```cpp
NavigateToPageSynchronously(url);
// The page has loaded, but may not be distilled yet.
EXPECT_TRUE(content::ExecJs(web_contents, "document.querySelector('h1')"));
```

Good:

```cpp
NavigateToPageSynchronously(url);
WaitDistilled();  // Wait for distillation before interacting with content
EXPECT_TRUE(content::ExecJs(web_contents, "document.querySelector('h1')"));
```
