---
name: brave/no-run-until-idle
description:
  Tests must never wait with RunLoop::RunUntilIdle(); they wait for the specific
  condition or callback instead.
rubric: testing
severity: blocking
---

**Rule.** NEVER call `base::RunLoop().RunUntilIdle()`, or any other
`RunUntilIdle()`, to wait for asynchronous work in a test. Wait for the specific
signal the test needs instead: `base::test::RunUntil()` for a C++ condition,
`base::test::TestFuture` for a callback, or an observer for an event.

**Why.** `RunUntilIdle()` returns when the current task queues are empty, not
when the awaited work is done: it returns too early when that work runs on
another sequence or posts a delayed task, and runs too long when unrelated tasks
keep the queue busy. Chromium's testing guidelines forbid it because the tests
it makes are flaky.

## Examples

Bad:

```cpp
service_->FetchData();
base::RunLoop().RunUntilIdle();
EXPECT_TRUE(service_->has_data());
```

Good:

```cpp
service_->FetchData();
ASSERT_TRUE(base::test::RunUntil([&]() { return service_->has_data(); }));
```
