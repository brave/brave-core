---
name: brave/remove-unused-includes
description:
  'A change must not leave #include lines for headers the file does not use, and
  every include of a new file is checked.'
rubric: readability
severity: should_fix
---

**Rule.** Remove every `#include` whose header the file no longer uses,
including those left behind when a change removes the code that needed them. In
a file the change adds, check each include against the code: NEVER keep one
copied from the file it was modelled on without a use.

**Why.** An unused include costs compile time in every file that includes the
one carrying it, adds a false dependency between targets, and hides which
headers the code really needs.

## Examples

Bad:

```cpp
// my_service.cc, which uses nothing from string_util.h.
#include "base/strings/string_util.h"
#include "brave/components/foo/my_service.h"
```

Good:

```cpp
#include "brave/components/foo/my_service.h"
```
