---
name: brave/no-include-order-review-comments
description:
  'A reviewer never comments on the order of #include lines, including where
  platform-specific includes are placed; formatting tools handle it.'
rubric: readability
severity: nit
---

**Rule.** NEVER report the order of `#include` lines in review: not their
sorting, not their grouping, and not where a platform-specific section sits.
Authors put platform-specific includes in their own section below the standard
includes, in the normal order within it, and `clang-format` and lint enforce the
rest. Whether an include is needed at all is a different question, which other
rules cover.

**Why.** Include ordering is mechanical: `clang-format` sorts includes and lint
reports what it cannot fix, so review comments about it only add noise.

## Examples

Bad:

```cpp
// Review comment: "base/ includes should come before brave/ includes."
#include "brave/components/foo/foo.h"
#include "base/strings/string_util.h"
```

Good:

```cpp
// No comment on the order; clang-format sorts it.
#include "brave/components/foo/foo.h"
#include "base/strings/string_util.h"
```
