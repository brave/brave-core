---
name: brave/synchronize-tests-on-the-awaited-condition
description:
  A test waits for the actual condition, callback or event it depends on, with
  RunUntil(), TestFuture, an observer or a quit closure, and never for an
  arbitrary amount of time.
rubric: testing
severity: blocking
---

**Rule.** When a test, or a fix to a flaky test, has to wait for asynchronous
work, it MUST wait on the real synchronization point: the condition, callback or
event the test depends on. Use `base::test::RunUntil()` for a C++ condition,
`base::test::TestFuture` for a callback, an observer (such as a
`content::WebContentsObserver`) for an event, or a run loop quit by a closure.
NEVER wait for an arbitrary time instead: `base::PlatformThread::Sleep()`, a
fixed delay, or a delayed task that quits a run loop. A fix to a flaky test
names what races, which side should happen first, and how the new wait removes
the race whatever the timing.

**Why.** A fixed wait only works when the awaited work happens to finish within
it, so the test still races: it fails on slower bots and sanitizer builds, and
wastes time on fast ones. Waiting for the condition itself is deterministic.

## Examples

Bad:

```cpp
NavigateToPageSynchronously(url);
base::PlatformThread::Sleep(base::Seconds(1));
EXPECT_EQ(tab_helper()->PageDistillState(), DistillState::kDistilled);
```

Good:

```cpp
NavigateToPageSynchronously(url);
ASSERT_TRUE(base::test::RunUntil([&]() {
  return tab_helper()->PageDistillState() == DistillState::kDistilled;
}));
```
