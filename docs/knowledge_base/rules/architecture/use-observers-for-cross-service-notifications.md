---
name: brave/use-observers-for-cross-service-notifications
description:
  'A service tells others about its events through an observer interface, not
  direct calls into each interested service.'
rubric: design
severity: should_fix
---

**Rule.** When other services need to react to something a service does, give it
an observer interface (a `base::ObserverList` of `Observer`s) and have the
interested services observe it. Do not add direct calls from the service into
each of them.

**Why.** Direct calls make the notifying service depend on every service that
cares, and tie them together in both directions; an observer keeps the
dependency one-way.

## Examples

Bad:

```cpp
void WalletService::OnAccountsChanged() {
  RewardsServiceFactory::GetForProfile(profile_)->OnWalletAccountsChanged();
}
```

Good:

```cpp
void WalletService::OnAccountsChanged() {
  for (auto& observer : observers_) {
    observer.OnAccountsChanged();
  }
}
```
