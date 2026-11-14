---
name: brave/tests-must-exercise-the-code-they-claim
description:
  A test's test-only controls (globals, mocks, feature overrides) must not
  switch off the code path the test claims to verify.
rubric: testing
severity: should_fix
---

**Rule.** When a test sets a test-only global, mocks out a component or
overrides a feature, check that the code its name and comments say it verifies
still runs. A test MUST NOT disable the code under test and then assert that the
effect is absent; it drives the real behaviour (against a mock server, a fake,
an enabled feature) and asserts on that.

**Why.** A test that disables what it claims to test passes whatever that code
does, so it proves nothing while giving the impression of coverage.

## Examples

Bad:

```cpp
// "The stats updater should not reach the endpoint", but stats are off.
void SetUp() override { g_disable_stats_for_testing = true; }
```

Good:

```cpp
// The stats updater runs, and the mock server records what it receives.
void SetUp() override {
  embedded_test_server()->RegisterRequestMonitor(base::BindRepeating(
      &StatsUpdaterTest::OnRequest, base::Unretained(this)));
  ASSERT_TRUE(embedded_test_server()->Start());
}
```
