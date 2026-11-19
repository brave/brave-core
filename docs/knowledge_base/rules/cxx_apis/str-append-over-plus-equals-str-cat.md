---
name: brave/str-append-over-plus-equals-str-cat
description:
  'Append to an existing string with base::StrAppend(&str, {...}), not str +=
  base::StrCat({...}).'
rubric: perf
severity: should_fix
---

**Rule.** To append several pieces to a string that already exists, call
`base::StrAppend(&str, {piece1, piece2, ...})`. Do not write
`str += base::StrCat({...})`.

**Why.** `base::StrCat()` builds a temporary string that `+=` then copies into
the target. `base::StrAppend()` grows the target once and writes the pieces into
it directly, avoiding the extra allocation and copy.

## Examples

Bad:

```cpp
result += base::StrCat({kOpenTag, "\n", "=== METADATA ===\n"});
```

Good:

```cpp
base::StrAppend(&result, {kOpenTag, "\n", "=== METADATA ===\n"});
```
