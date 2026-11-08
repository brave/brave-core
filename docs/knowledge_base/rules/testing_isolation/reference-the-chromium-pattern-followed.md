---
name: brave/reference-the-chromium-pattern-followed
description:
  Test code that follows a Chromium pattern names the Chromium file it comes
  from in a comment.
rubric: readability
severity: nit
---

**Rule.** When a test adopts a pattern from Chromium's tests (a
`MutationObserver` wait, an observer-based wait, a particular fixture setup),
the comment explaining it names the Chromium file it follows, such as
`// Pattern from service_worker_internals_ui_browsertest.cc.`. The commit
message can carry the reference too, but the code comment is the one the next
reader sees.

**Why.** The reference tells a reader why the code looks the way it does and
where to find the original, and it lets the next fix compare against the
upstream pattern instead of rediscovering it.

## Examples

Bad:

```cpp
// Waits for the DOM to update.
static constexpr char kWaitForTextScript[] = R"(
  // ...
)";
```

Good:

```cpp
// NOTE: Replace() is an IPC to the renderer that updates the DOM
// asynchronously. We use a MutationObserver to wait for the DOM to update
// to the expected value before checking.
// Pattern from service_worker_internals_ui_browsertest.cc.
static constexpr char kWaitForTextScript[] = R"(
  // ...
)";
```
