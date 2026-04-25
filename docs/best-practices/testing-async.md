# Async Testing Patterns

<a id="TA-001"></a>

## Root Cause Analysis

<a id="TA-002"></a>

### ❌ NEVER Make Timing-Based "Fixes"

**DO NOT make changes that "fix" the test by altering execution timing.**

These "fixes" may make the problem disappear locally, but the underlying race condition will inevitably return. This is the most common type of fake fix.

**BANNED - Any change that works by altering timing rather than providing proper synchronization:**

```cpp
// ❌ WRONG - Adding logging to "fix" a race condition
LOG(INFO) << "Debug output";  // Changes timing!
std::cout << "Checking state" << std::endl;  // Race condition hidden, not fixed
VLOG(1) << "Operation completed";  // Still just hiding the problem

// ❌ WRONG - Meaningless operations that change timing
volatile int dummy = 0;
for (int i = 0; i < 100; i++) { dummy++; }  // Delay tactic, not a fix
auto unused = SomeGetter();  // Adds execution time
std::this_thread::yield();  // Still a timing hack

// ❌ WRONG - Reordering unrelated code hoping it helps
SomeUnrelatedFunction();  // Accidentally changes timing
ActualTestCode();

// ❌ WRONG - Adding includes that change compilation/execution order
#include "some_header.h"  // If this "fixes" it, you haven't found the real problem

// ❌ WRONG - Refactoring that accidentally changes execution order
ExtractMethodThatChangesWhenThingsRun();  // Same code, different timing

// ❌ WRONG - ANY OTHER CHANGE where you can't explain the synchronization
// If removing it makes the test flaky again, but you don't know WHY it works,
// it's a fake fix that will eventually break
```

**This list is not exhaustive.** The key principle: if your change works by altering when things execute rather than by adding proper synchronization, it's unacceptable.

**Why these "fixes" are unacceptable:**
1. **They hide the problem, not solve it** - The race condition still exists
2. **They're compiler/optimization dependent** - May work in debug but fail in release builds
3. **They're platform dependent** - May work on your machine but fail in CI
4. **They'll break again** - As soon as something else changes timing (new code, different CPU, etc.)

<a id="TA-003"></a>

### ✅ Proper Root Cause Analysis

**REQUIRED approach for all test fixes:**

1. **Identify the actual race condition:**
   - What two things are racing?
   - Which happens first? Which should happen first?
   - What's the synchronization mechanism (or lack thereof)?

2. **Find the real synchronization point:**
   - Use proper wait mechanisms (`base::test::RunUntil()`, `TestFuture`, observers)
   - Wait for the actual condition you care about, not arbitrary time
   - Use explicit synchronization primitives (callbacks, run loops with quit closures)

3. **Verify the fix addresses the root cause:**
   - Can you explain WHY the test was flaky?
   - Can you explain HOW your fix eliminates the race?
   - Would your fix work regardless of timing variations?

**GOOD - Actual fixes that address root causes:**

```cpp
// ✅ CORRECT - Wait for the actual condition
ASSERT_TRUE(base::test::RunUntil([&]() {
  return tab_helper()->PageDistillState() == DistillState::kDistilled;
}));

// ✅ CORRECT - Use TestFuture to synchronize on callback
TestFuture<Result> future;
DoAsyncOperation(future.GetCallback());
const Result& result = future.Get();  // Blocks until callback fires

// ✅ CORRECT - Use observer pattern for event notification
class MyObserver : public content::WebContentsObserver {
  void DidFinishNavigation(NavigationHandle* handle) override {
    if (handle->IsSameDocument()) {
      run_loop_.Quit();
    }
  }
  base::RunLoop run_loop_;
};
```

<a id="TA-004"></a>

### Rule of Thumb

**If removing your "fix" would make the test flaky again, but you can't explain WHY it fixes the race condition, it's not a real fix.**

Real fixes are:
- Deterministic (work every time)
- Explainable (you can describe the synchronization mechanism)
- Robust (work across different timing conditions, platforms, and build types)

---

<a id="TA-005"></a>

## ❌ NEVER Use RunUntilIdle()

**DO NOT use `RunLoop::RunUntilIdle()` for asynchronous testing.**

This is explicitly forbidden by Chromium style guide because it causes flaky tests:
- May run too long and timeout
- May return too early if events depend on different task queues
- Creates unreliable, non-deterministic tests

**Reference:** [Chromium C++ Testing Best Practices](https://www.chromium.org/chromium-os/developer-library/guides/testing/cpp-writing-tests/)

---

<a id="TA-006"></a>

## ✅ Use base::test::RunUntil() for C++ Conditions

**GOOD - When checking C++ state:**
```cpp
ASSERT_TRUE(base::test::RunUntil([th]() {
  return speedreader::DistillStates::IsDistilled(th->PageDistillState());
}));
```

**GOOD - When checking object properties:**
```cpp
ASSERT_TRUE(base::test::RunUntil([this]() {
  return tab_helper()->speedreader_bubble_view() != nullptr;
}));
```

---

<a id="TA-007"></a>

## ❌ CRITICAL: Never Use EvalJs Inside RunUntil()

**DO NOT call `content::EvalJs()` or `content::ExecJs()` inside `base::test::RunUntil()` lambdas.**

This causes DCHECK failures on macOS arm64 due to nested run loop issues.

**BAD - Causes DCHECK failure:**
```cpp
// ❌ WRONG - Nested run loops!
ASSERT_TRUE(base::test::RunUntil([&]() {
  return content::EvalJs(web_contents, "!!document.getElementById('foo')")
      .ExtractBool();
}));
```

**Error you'll see on macOS arm64:**
```
FATAL:base/message_loop/message_pump_apple.mm:389]
DCHECK failed: stack_.size() < static_cast<size_t>(nesting_level_)
```

**Why it fails:**
1. `base::test::RunUntil()` starts a run loop to poll the condition
2. Inside that loop, `content::EvalJs()` starts **another** run loop to execute JavaScript
3. This creates **nested run loops**, which triggers a DCHECK on macOS

---

<a id="TA-008"></a>

## General Rule: Avoid Nested Run Loops

**Any operation that creates its own run loop should NOT be called inside `base::test::RunUntil()`:**

- ❌ `content::EvalJs()` - creates run loop
- ❌ `content::ExecJs()` - creates run loop
- ❌ IPC operations that wait for responses - create run loops
- ✅ Direct C++ state checks - safe
- ✅ Simple getter methods - safe
- ✅ Checking object properties - safe

---

<a id="TA-009"></a>

## ✅ ALWAYS Use `base::test::TestFuture` Over RunLoop for Callbacks

**REQUIRED for callback-based operations.** `TestFuture` is more concise and less error-prone than `RunLoop` patterns. Reviewers actively enforce this — PRs using `RunLoop` + `BindLambdaForTesting` + `Quit()` when `TestFuture` would work will receive review comments requesting changes.

```cpp
// ❌ WRONG - verbose RunLoop + lambda pattern (will be flagged in review)
base::RunLoop run_loop;
bool result = false;
service->CheckPurchaseState(
    base::BindLambdaForTesting([&](bool is_purchased) {
      result = is_purchased;
      run_loop.Quit();
    }));
run_loop.Run();
EXPECT_FALSE(result);

// ✅ CORRECT - TestFuture is simpler and cleaner
base::test::TestFuture<bool> future;
service->CheckPurchaseState(future.GetCallback());
EXPECT_FALSE(future.Get());
```

**Always use `TestFuture` when the async operation takes a `base::OnceCallback`.** The only exception is when you need manual run loop control that `TestFuture` cannot provide (e.g., testing timeout behavior or cancellation).

---

<a id="TA-010"></a>

## ✅ Mojo Message Ordering for Test Synchronization

**Mojo processes messages in order on a given interface.** Making a synchronous mojo call through an interface guarantees that all prior async messages on that same interface have been processed. This is a precise alternative to polling for cross-process test synchronization.

```cpp
// ❌ WRONG - polling with arbitrary delay
base::PlatformThread::Sleep(base::Milliseconds(100));

// ✅ CORRECT - use sync mojo call to ensure prior messages processed
// If Init() was sent async on the Solana provider interface,
// calling IsConnected() synchronously ensures Init() completed first
// (since mojo processes messages in order on a given interface)
EXPECT_EQ(true, content::EvalJs(web_contents,
    "window.braveSolana.isConnected"));
```

---

<a id="TA-011"></a>

## Alternative: QuitClosure() + Run()

**For manual control when TestFuture doesn't fit:**
```cpp
base::RunLoop run_loop;
object_under_test.DoSomethingAsync(run_loop.QuitClosure());
run_loop.Run();  // Waits specifically for this closure
```

---

<a id="TA-012"></a>

## ✅ Wait for Mojo Completion Before EvalJs Assertions

**In browser tests, do not call `EvalJs` immediately after an async Mojo operation to check its effects.** The mojo message may not have been processed yet. Use polling (`RunUntil` with a C++ state check) or a `TestFuture` to synchronize on the mojo response before asserting via JavaScript.

```cpp
// ❌ WRONG - EvalJs races with pending mojo response
service->UpdateSettings(new_settings);
EXPECT_EQ(true, content::EvalJs(web_contents, "isSettingEnabled()"));

// ✅ CORRECT - wait for mojo to complete first
base::test::TestFuture<bool> future;
service->UpdateSettings(new_settings, future.GetCallback());
EXPECT_TRUE(future.Get());
EXPECT_EQ(true, content::EvalJs(web_contents, "isSettingEnabled()"));
```
