---
name: brave/use-test-future-for-callbacks
description:
  A test waiting for an operation that takes a base::OnceCallback uses
  base::test::TestFuture, not a RunLoop with a BindLambdaForTesting() lambda
  that stores the result and calls Quit().
rubric: testing
severity: should_fix
---

**Rule.** When the asynchronous operation under test takes a
`base::OnceCallback`, wait for it with `base::test::TestFuture`: pass
`future.GetCallback()` and read the result with `future.Get()` (or
`future.Take()`, `future.Wait()`). Do not declare a `base::RunLoop`, bind a
`base::BindLambdaForTesting()` lambda that copies the result into a local and
calls `run_loop.Quit()`, then `Run()` the loop. The only exception is a test
that needs manual run loop control `TestFuture` cannot provide, such as testing
timeout behaviour or cancellation.

**Why.** `TestFuture` is shorter and less error-prone than the `RunLoop` and
lambda boilerplate: it captures the arguments type-safely, waits for exactly
that callback, and fails clearly instead of hanging when it never runs.

## Examples

Bad:

```cpp
base::RunLoop run_loop;
bool result = false;
service->CheckPurchaseState(
    base::BindLambdaForTesting([&](bool is_purchased) {
      result = is_purchased;
      run_loop.Quit();
    }));
run_loop.Run();
EXPECT_FALSE(result);
```

Good:

```cpp
base::test::TestFuture<bool> future;
service->CheckPurchaseState(future.GetCallback());
EXPECT_FALSE(future.Get());
```
