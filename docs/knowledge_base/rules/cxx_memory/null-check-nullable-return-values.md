---
name: brave/null-check-nullable-return-values
description:
  'Return values of methods that can return nullptr are null-checked before
  being dereferenced, even when the argument looks valid.'
rubric: correctness
severity: blocking
---

**Rule.** Before dereferencing the result of a method that can return `nullptr`,
check it, even when the ID or key passed in is non-null or seems valid. Do not
chain a call onto such a method (`a->GetB()->DoC()`): store the intermediate
result and check it. Common Chromium APIs that return `nullptr` are `GetTask()`,
`FindWebContentsById()`, `FromWebContents()`, `FromBrowserContext()`,
`GetTabById()` and `GetWindowById()`.

**Why.** A valid ID or key does not guarantee the lookup succeeds: the object
may have been destroyed, removed or never created. Upstream Chromium APIs change
the lifetimes behind such lookups without notice, and the dereference then
crashes.

## Examples

Bad:

```cpp
if (!task_id_.is_null()) {
  actor_service_->GetTask(task_id_)->Pause(false);  // crash if task was removed
}
```

Good:

```cpp
if (!task_id_.is_null()) {
  if (auto* task = actor_service_->GetTask(task_id_)) {
    task->Pause(false);
  }
}
```
