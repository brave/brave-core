---
name: brave/no-commented-out-code
description:
  'A change does not leave commented-out code, dead #include lines or
  development leftovers behind.'
rubric: readability
severity: should_fix
---

**Rule.** Delete code instead of commenting it out: no commented-out statements,
functions or `#include` lines, and no other leftovers of development, such as
debugging code, before the change merges.

**Why.** Commented-out code adds noise, confuses readers about whether it still
matters, and is never cleaned up. Version control keeps it if it is ever needed
again.

## Examples

Bad:

```cpp
// #include "brave/components/old_feature/old_header.h"
// void OldMethod() { ... }
```

Good:

```cpp
// (Both lines removed.)
```
