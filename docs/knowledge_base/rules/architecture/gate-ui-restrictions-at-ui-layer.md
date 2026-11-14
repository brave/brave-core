---
name: brave/gate-ui-restrictions-at-ui-layer
description:
  'Core utility functions such as IsFeatureEnabled() do not check UI state;
  UI-specific restrictions are applied in the UI layer.'
rubric: design
severity: should_fix
---

**Rule.** Do not make a core utility function, such as a feature's
`Is*Enabled()` check, depend on UI state like whether a WebUI is available.
Check the UI condition in the UI code that hides or shows the UI.

**Why.** Core checks are used by services and tests that have nothing to do with
the UI; coupling them to UI state makes those fail unexpectedly and ties the
core layer to the UI.

## Examples

Bad:

```cpp
bool IsZCashShieldedTransactionsEnabled() {
  return IsZCashEnabled() && kShieldedEnabled.Get() && IsWalletWebUIEnabled();
}
```

Good:

```cpp
bool IsZCashShieldedTransactionsEnabled() {
  return IsZCashEnabled() && kShieldedEnabled.Get();
}
// UI layer checks: if (!IsWalletWebUIEnabled()) { hide shielded tx UI }
```
