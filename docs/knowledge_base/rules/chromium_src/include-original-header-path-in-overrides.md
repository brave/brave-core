---
name: brave/include-original-header-path-in-overrides
description:
  A chromium_src override includes a header by its upstream path, never by its
  brave/chromium_src/ path.
rubric: correctness
severity: blocking
---

**Rule.** In a `chromium_src/` override, NEVER `#include` (or `#import`) a
header as `"brave/chromium_src/<path>"`. Include it by its upstream path,
`"<path>"`, whether or not Brave overrides that header.

**Why.** The build already resolves an upstream include path to its
`chromium_src/` override when one exists, so the upstream path picks up the
override everywhere. Spelling out the `chromium_src` path bypasses that
mechanism and ties the include to the override's location.

## Examples

Bad:

```cpp
#include "brave/chromium_src/net/proxy_resolution/proxy_resolution_service.h"
```

Good:

```cpp
#include "net/proxy_resolution/proxy_resolution_service.h"
```
