---
name: brave/scoped-trace-caller-location-in-test-helpers
description:
  A browser test helper that asserts takes a base::Location defaulted to the
  caller and opens it with SCOPED_TRACE.
rubric: testing
severity: should_fix
---

**Rule.** A browser test helper function that contains `EXPECT_*` or `ASSERT_*`
checks takes a trailing `base::Location location = base::Location::Current()`
parameter and starts with `SCOPED_TRACE(location.ToString())`, so each failure
names the line that called the helper.

**Why.** gtest reports a failure at the line of the assertion, which is inside
the helper. When several tests, or several calls in one test, use the same
helper, the failure does not say which call failed until the caller's location
is traced.

## Examples

Bad:

```cpp
void VerifyTabCount(Browser* browser, int expected) {
  EXPECT_EQ(browser->tab_strip_model()->count(), expected);
}
```

Good:

```cpp
void VerifyTabCount(Browser* browser,
                    int expected,
                    base::Location location = base::Location::Current()) {
  SCOPED_TRACE(location.ToString());
  EXPECT_EQ(browser->tab_strip_model()->count(), expected);
}
```
