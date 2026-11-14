---
name: brave/multiply-before-dividing-in-percentages
description:
  'An integer percentage is computed as (part * 100) / total, multiplying before
  dividing.'
rubric: correctness
severity: should_fix
---

**Rule.** In integer arithmetic, compute a percentage as `(part * 100) / total`.
NEVER divide first, as in `(part / total) * 100`.

**Why.** Integer division truncates: when `part < total`, `part / total` is 0
and the percentage is always 0.

## Examples

Bad:

```cpp
int pct = (used / total) * 100;
```

Good:

```cpp
int pct = (used * 100) / total;
```
