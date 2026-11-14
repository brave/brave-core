---
name: brave/verify-side-effects-do-not-happen-on-failure
description:
  A test of an error or failure path also asserts that the side effects the
  failure must not cause did not happen.
rubric: testing
severity: should_fix
---

**Rule.** When a test drives an error or failure path, it asserts both the error
and the absence of the side effects that a failure must not cause: an observer
not notified (`EXPECT_CALL(...).Times(0)`), state not changed, a request not
sent, a pref not written. Flag a failure-path test that checks only the returned
error.

**Why.** A common bug in error handling is doing part of the work before
failing. A test that only checks the error passes while the failure still
notifies observers or corrupts state.

## Examples

Bad:

```cpp
EXPECT_FALSE(result.has_value());
```

Good:

```cpp
EXPECT_CALL(observer, OnConversationTitleChanged(_)).Times(0);
EXPECT_FALSE(result.has_value());
```
