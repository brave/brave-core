---
name: brave/keyed-service-cleanup-in-shutdown
description:
  'KeyedService implementations clean up their owned resources in Shutdown(),
  not only in the destructor.'
rubric: correctness
severity: should_fix
---

**Rule.** A `KeyedService` subclass releases the resources it owns, and drops
its references to other services, in an override of `Shutdown()`. Do not leave
that cleanup to the destructor alone.

**Why.** The keyed service graph has dependencies that need an orderly teardown:
every service's `Shutdown()` runs, in dependency order, before any service is
destroyed. Cleanup left to the destructor runs after the services it depends on
may already be gone.

## Examples

Bad:

```cpp
MyService::~MyService() {
  fetcher_.reset();
  history_service_ = nullptr;
}
```

Good:

```cpp
void MyService::Shutdown() {
  fetcher_.reset();
  history_service_ = nullptr;
}
```
