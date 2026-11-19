---
name: brave/no-raw-ptr-vector-bound-in-callbacks
description:
  'Async callbacks must never bind a std::vector<raw_ptr<T>>; bind a
  std::vector<base::WeakPtr<T>> instead.'
rubric: safety
severity: blocking
---

**Rule.** NEVER bind or capture a `std::vector<raw_ptr<T>>` in an asynchronous
callback. Convert it to a `std::vector<base::WeakPtr<T>>` before binding, and
let the callback skip the entries that have become null.

**Why.** The pointed-to objects can be destroyed before the callback runs, so
the raw pointers dangle. `WeakPtr`s become null instead.

## Examples

Bad:

```cpp
base::BindOnce(&OnComplete, std::move(raw_ptr_vector));
```

Good:

```cpp
std::vector<base::WeakPtr<Tab>> weak_tabs;
for (auto* tab : tabs) {
  weak_tabs.push_back(tab->GetWeakPtr());
}
base::BindOnce(&OnComplete, std::move(weak_tabs));
```
