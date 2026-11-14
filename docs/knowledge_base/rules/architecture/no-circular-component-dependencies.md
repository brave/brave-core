---
name: brave/no-circular-component-dependencies
description:
  'Component dependencies form a tree: if component A depends on component B, B
  must never depend on A, directly or transitively.'
rubric: design
severity: blocking
---

**Rule.** When a component gains a dependency (an `#include`, a GN dep, a DEPS
rule) on another component, that other component MUST NOT already depend on it,
directly or through others. Break such a cycle with a delegate interface the
dependent component declares, or an observer, instead.

**Why.** A cycle ties the components into one unit that cannot be built, tested
or reused separately, and GN rejects cycles between targets.

## Examples

Bad:

```gn
# components/brave_wallet/browser/BUILD.gn
deps = [ "//brave/components/brave_rewards/core" ]
# components/brave_rewards/core/BUILD.gn
deps = [ "//brave/components/brave_wallet/browser" ]
```

Good:

```cpp
// components/brave_wallet/browser/wallet_delegate.h, implemented by Rewards
class WalletDelegate {
 public:
  virtual void OnBalanceChanged() = 0;
};
```
