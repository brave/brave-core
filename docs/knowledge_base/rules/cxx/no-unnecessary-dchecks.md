---
name: brave/no-unnecessary-dchecks
description:
  'Code does not assert conditions that cannot fail, such as
  DCHECK(g_browser_process) in a running browser.'
rubric: readability
severity: nit
---

**Rule.** Do not add a `DCHECK()` (or `CHECK()`) of a condition that cannot be
false where it is checked, such as `DCHECK(g_browser_process)` in browser code,
which could not be running without it.

**Why.** An assertion that can never fire documents nothing and adds noise that
hides the assertions that matter.

## Examples

Bad:

```cpp
void BraveBrowserProcessImpl::Init() {
  DCHECK(g_browser_process);
  ...
}
```

Good:

```cpp
void BraveBrowserProcessImpl::Init() {
  ...
}
```
