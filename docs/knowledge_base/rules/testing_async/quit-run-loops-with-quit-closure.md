---
name: brave/quit-run-loops-with-quit-closure
description:
  When TestFuture does not fit and a test runs a base::RunLoop by hand, it hands
  run_loop.QuitClosure() to the awaited operation and calls Run(), so the loop
  waits for that closure specifically.
rubric: testing
severity: should_fix
---

**Rule.** Use a hand-driven `base::RunLoop` only for manual control that
`base::test::TestFuture` cannot give. Then pass `run_loop.QuitClosure()` as the
completion callback of the operation being awaited (or call it from the observer
method that sees the awaited event) and call `run_loop.Run()`, so the loop ends
exactly when that operation completes. Do not end the loop any other way, such
as a delayed task or running until idle.

**Why.** A quit closure tied to the awaited operation makes `Run()` wait for
that operation specifically; anything else makes the loop end at an unrelated
moment and the test races.

## Examples

Bad:

```cpp
base::RunLoop run_loop;
object_under_test.DoSomethingAsync(base::DoNothing());
base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
    FROM_HERE, run_loop.QuitClosure(), base::Seconds(1));
run_loop.Run();
```

Good:

```cpp
base::RunLoop run_loop;
object_under_test.DoSomethingAsync(run_loop.QuitClosure());
run_loop.Run();  // Waits specifically for this closure
```
