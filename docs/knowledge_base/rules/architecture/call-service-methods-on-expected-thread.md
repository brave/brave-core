---
name: brave/call-service-methods-on-expected-thread
description:
  'Before calling a service method, verify which thread or sequence it expects,
  especially for ad-block and shields services, since a call from the wrong
  thread crashes.'
rubric: correctness
severity: blocking
---

**Rule.** When code calls a service method, especially on an ad-block or shields
service, or posts a task that will, check that the call happens on the thread or
sequence the method expects (the UI thread, the service's task runner). A call
from another thread MUST be posted to the right one.

**Why.** Services check, or assume, the sequence they run on; a call from the
wrong thread trips their sequence checks or races their state, and crashes.

## Examples

Bad:

```cpp
// On a ThreadPool sequence:
foo_service_->UpdateRules(rules);  // FooService expects the UI thread
```

Good:

```cpp
content::GetUIThreadTaskRunner({})->PostTask(
    FROM_HERE,
    base::BindOnce(&FooService::UpdateRules, foo_service_weak_, rules));
```
