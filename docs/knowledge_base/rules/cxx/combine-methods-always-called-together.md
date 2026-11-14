---
name: brave/combine-methods-always-called-together
description:
  'Two methods always called one after the other, especially from a patch, are
  combined into one method.'
rubric: design
severity: should_fix
---

**Rule.** When a change adds two calls that must always run together in the same
order, in a patch above all, provide one method that makes both calls, and call
that.

**Why.** In a patch, every added line is a potential rebase conflict, so one
call is cheaper to maintain than two. Everywhere, a single method keeps a caller
from making one call and forgetting the other.

## Examples

Bad:

```cpp
+SignBinaries(params);
+CopyPreSignedBinaries(params);
```

Good:

```cpp
+PrepareBinaries(params);  // Calls both.
```
