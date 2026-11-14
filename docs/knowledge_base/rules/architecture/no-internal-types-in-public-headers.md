---
name: brave/no-internal-types-in-public-headers
description:
  "A component's public headers never expose its internal implementation types;
  they use its public API types, such as its mojom types."
rubric: design
severity: blocking
---

**Rule.** A component's public header MUST NOT include the component's internal
headers or use its internal types (`internal::` namespaces, engine or database
classes) in declarations. Use the component's public API types, such as the
mojom types it exposes.

**Why.** Internal types in a public header make every user depend on the
component's implementation, so the internals can no longer change without
touching its clients.

## Examples

Bad:

```cpp
#include "brave/components/brave_rewards/core/engine/database/database_publisher_info.h"
void DoSomething(internal::database::DatabasePublisherInfo info);
```

Good:

```cpp
#include "brave/components/brave_rewards/core/mojom/rewards.mojom.h"
void DoSomething(mojom::PublisherInfoPtr info);
```
