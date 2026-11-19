---
name: brave/descriptive-test-names
description:
  Test names describe the scenario and expected outcome well enough to
  understand without reading the body, however long that makes them.
rubric: readability
severity: nit
---

**Rule.** Name each test after the scenario it exercises and the outcome it
expects, such as `SendTransaction_ReturnsErrorForInsufficientBalance`. Flag
names like `TestSend`, `Error` or `Works` that a reader cannot interpret without
opening the test body.

**Why.** The test name is all a failure report and a filter list show. A name
that states the scenario tells the reader what broke straight from the log; a
terse one sends them into the code.

## Examples

Bad:

```cpp
TEST_F(WalletServiceTest, TestSend) {}
TEST_F(WalletServiceTest, Error) {}
```

Good:

```cpp
TEST_F(WalletServiceTest, SendTransaction_ReturnsErrorForInsufficientBalance) {}
TEST_F(WalletServiceTest, GetTokenBalances_HandlesNetworkTimeoutGracefully) {}
```
