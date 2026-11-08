---
name: brave/result-codes-over-bool
description:
  'An operation that can fail reports its outcome with a result enum rather than
  a bool.'
rubric: design
severity: should_fix
---

**Rule.** When a function, or the callback it runs, reports whether an operation
succeeded, use an `enum class` of result codes rather than `bool`, so that it
can say why it failed.

**Why.** A `bool` cannot tell the caller, its logs or its metrics which failure
happened, and adding a second failure later means changing every caller.

## Examples

Bad:

```cpp
bool SaveWallet(const Wallet& wallet);
```

Good:

```cpp
enum class SaveWalletResult {
  kSuccess,
  kInvalidWallet,
  kWriteFailed,
};

SaveWalletResult SaveWallet(const Wallet& wallet);
```
