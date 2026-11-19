---
name: brave/include-guard-name-from-path
description:
  "A header's include guard is its full path from the source root, uppercased
  with / and . replaced by _, plus a trailing _."
rubric: readability
severity: nit
---

**Rule.** Name a header's include guard after its full path from the source root
(`src/`), uppercased, with `/` and `.` replaced by `_` and a trailing `_`:
`brave/components/foo/bar.h` is guarded by `BRAVE_COMPONENTS_FOO_BAR_H_`, and
the `#endif` repeats the name in a comment. A header that is moved or renamed
gets its guard renamed too.

**Why.** Guards derived from the path are unique across the tree; a guard copied
from another header, or left behind by a move, silently excludes one of the two
headers.

## Examples

Bad:

```cpp
// brave/components/foo/bar.h
#ifndef BAR_H_
#define BAR_H_
```

Good:

```cpp
// brave/components/foo/bar.h
#ifndef BRAVE_COMPONENTS_FOO_BAR_H_
#define BRAVE_COMPONENTS_FOO_BAR_H_

#endif  // BRAVE_COMPONENTS_FOO_BAR_H_
```
