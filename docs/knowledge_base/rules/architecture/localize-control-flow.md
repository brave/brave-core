---
name: brave/localize-control-flow
description:
  'An operation is passed the state it needs as parameters, instead of querying
  non-local state (global registries, managers) in the middle of its execution.'
rubric: design
severity: should_fix
---

**Rule.** Keep control flow local: pass an operation the state it works on as
parameters. Do not have it fetch that state from global registries, managers or
other objects partway through.

**Why.** State fetched mid-execution can change under the operation, often
because the fetch calls back into the same object; the operation's real inputs
are hidden from its callers and from tests.

## Examples

Bad:

```cpp
void ProcessItem(int id) {
  auto item = global_registry_->GetItem(id);
  auto config = settings_manager_->GetConfig();
  DoWork(item, config);
}
```

Good:

```cpp
void ProcessItem(const Item& item, const Config& config) {
  DoWork(item, config);
}
```
