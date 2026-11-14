---
name: brave/encapsulate-cleanup-in-owning-class
description:
  'Cleanup of a resource, such as deleting its files, is a method of the class
  that owns the resource, not code repeated by its callers.'
rubric: design
severity: should_fix
---

**Rule.** When a resource needs cleaning up (its files deleted, its state
reset), give the class that owns it a method that does so, and call that. Do not
have callers fetch the resource's paths or internals and clean it up themselves.

**Why.** The owner knows what its resource consists of; cleanup spread across
callers duplicates that knowledge, and misses whatever the owner adds later.

## Examples

Bad:

```cpp
base::FilePath path = tor_client_updater()->GetExecutablePath();
base::DeleteFile(path);
```

Good:

```cpp
tor_client_updater()->Cleanup();
```
