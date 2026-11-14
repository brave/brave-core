---
name: brave/wait-for-mojo-before-evaljs-assertions
description:
  A browser test does not assert with EvalJs() right after an async Mojo
  operation; it first synchronizes on the Mojo response with a TestFuture or a
  RunUntil() C++ state check.
rubric: testing
severity: blocking
---

**Rule.** In browser tests, NEVER check the effect of an asynchronous Mojo call
with `content::EvalJs()` straight after making it. First synchronize on the Mojo
response: pass a `base::test::TestFuture` callback and `Get()` it, or poll the
C++ state the response changes with `base::test::RunUntil()`. Only then assert
through JavaScript.

**Why.** The Mojo message may not have been processed when `EvalJs()` runs, so
the assertion races with the pending response and the test is flaky.

## Examples

Bad:

```cpp
service->UpdateSettings(new_settings);
EXPECT_EQ(true, content::EvalJs(web_contents, "isSettingEnabled()"));
```

Good:

```cpp
base::test::TestFuture<bool> future;
service->UpdateSettings(new_settings, future.GetCallback());
EXPECT_TRUE(future.Get());
EXPECT_EQ(true, content::EvalJs(web_contents, "isSettingEnabled()"));
```
