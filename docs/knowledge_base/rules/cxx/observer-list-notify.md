---
name: brave/observer-list-notify
description:
  'Observers in a base::ObserverList are notified with
  observers_.Notify(&Observer::Method, ...), not a hand-written loop.'
rubric: readability
severity: nit
---

**Rule.** To call one method on every observer of a `base::ObserverList`, write
`observers_.Notify(&Observer::Method, args...)` instead of a range-for loop over
the list.

**Why.** `Notify()` says what the loop does in one line, and leaves the
iteration, including its reentrancy checks, to the list.

## Examples

Bad:

```cpp
for (auto& observer : observers_) {
  observer.OnPoliciesChanged();
}
```

Good:

```cpp
observers_.Notify(&Observer::OnPoliciesChanged);
```
