---
name: brave/no-timing-based-test-fixes
description:
  A flaky test must never be "fixed" by a change that only alters timing, such
  as added logging, dummy work, reordering or new includes; it needs real
  synchronization.
rubric: testing
severity: blocking
---

**Rule.** NEVER make a flaky test pass by changing when things execute rather
than by synchronizing them. This bans, among others: adding logging (`LOG`,
`VLOG`, `std::cout`), meaningless work that takes time (`volatile` counters,
unused getter calls, `std::this_thread::yield()`), reordering unrelated code,
adding includes, and refactoring that moves when code runs. It bans any other
change that works this way too, whether or not it is listed. A real fix is
deterministic, explainable and robust: if removing it would make the test flaky
again and nobody can explain WHY it removes the race, it is a fake fix, and the
change MUST add proper synchronization instead.

**Why.** A timing change hides the race condition without removing it. It
depends on the compiler, the optimization level and the platform, so it may pass
in debug builds or locally and fail in release builds or on CI, and it breaks
again as soon as other code, another CPU or another build changes the timing.

## Examples

Bad:

```cpp
service_->FetchData();
VLOG(1) << "Fetch started";  // Changes timing; the race is still there.
EXPECT_TRUE(service_->has_data());
```

Good:

```cpp
service_->FetchData();
ASSERT_TRUE(base::test::RunUntil([&]() { return service_->has_data(); }));
```
