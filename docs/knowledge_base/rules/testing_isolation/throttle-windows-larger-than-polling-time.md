---
name: brave/throttle-windows-larger-than-polling-time
description:
  A test that checks behaviour inside a throttle window uses a window much
  longer than the polling waits that run inside it.
rubric: testing
severity: blocking
---

**Rule.** When a test sets a throttle duration and then checks that something is
still throttled, the throttle MUST be much longer than the polling waits
(`WaitForSelectorBlocked()`, `NonBlockingDelay()`) that run before and during
the check. Flag a throttle window that the waits in the test could use up, such
as a 500 ms throttle checked after a poll that can take 300 ms and a further 250
ms delay.

**Why.** The throttle timer starts when the fetch happens, not when the test
starts checking, so every poll spends part of the window. A window close to the
polling time expires before the check on a slow bot, and the test flakes.

## Examples

Bad:

```cpp
const int kThrottleMs = 500;
WaitForSelectorBlocked();                   // May take 200-300ms.
NonBlockingDelay(base::Milliseconds(250));  // The window may have expired.
```

Good:

```cpp
const int kThrottleMs = 2000;
WaitForSelectorBlocked();                    // Takes ~200-300ms.
NonBlockingDelay(base::Milliseconds(1000));  // Still well within the window.
```
