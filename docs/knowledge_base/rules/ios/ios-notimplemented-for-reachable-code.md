---
name: brave/ios-notimplemented-for-reachable-code
description:
  'Code paths that can be reached but are not implemented yet use
  NOTIMPLEMENTED(); NOTREACHED() is only for paths that can never execute.'
rubric: safety
severity: blocking
---

**Rule.** NEVER put `NOTREACHED()` on a path that can run: a stub of an
interface method not implemented on iOS yet, a case the platform does not
support, a branch user input can reach. Use `NOTIMPLEMENTED()` there. Keep
`NOTREACHED()` for paths that truly never execute.

**Why.** `NOTREACHED()` crashes release builds, so a reachable one is a crash
for users; `NOTIMPLEMENTED()` only logs.

## Examples

Bad:

```cpp
void BraveWalletProviderDelegateBridge::ShowAccountCreation(
    mojom::CoinType type) {
  NOTREACHED();  // Called when a dApp requests an account.
}
```

Good:

```cpp
void BraveWalletProviderDelegateBridge::ShowAccountCreation(
    mojom::CoinType type) {
  NOTIMPLEMENTED();
}
```
