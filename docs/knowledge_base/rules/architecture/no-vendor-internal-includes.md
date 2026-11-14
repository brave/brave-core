---
name: brave/no-vendor-internal-includes
description:
  "Never #include a brave/vendor/ internal header by its full path; use the
  component's public API header."
rubric: design
severity: blocking
---

**Rule.** NEVER write `#include "brave/vendor/..."` to reach a vendored
library's internal header. Include the public header of the component that wraps
it.

**Why.** Internal headers are not part of the public API; including them by full
path bypasses GN visibility and couples the code to implementation details that
can change without notice.

## Examples

Bad:

```cpp
#include "brave/vendor/brave_base/random.h"
```

Good:

```cpp
#include "brave/components/brave_rewards/core/engine/util/random_util.h"
```
