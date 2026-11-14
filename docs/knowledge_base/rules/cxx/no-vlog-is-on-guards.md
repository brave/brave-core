---
name: brave/no-vlog-is-on-guards
description:
  'A VLOG is not wrapped in an if (VLOG_IS_ON(n)) check, which the macro already
  performs.'
rubric: readability
severity: nit
---

**Rule.** Do not guard a `VLOG(n)` with `if (VLOG_IS_ON(n))`. Write the
`VLOG(n)` alone.

**Why.** `VLOG` checks the level itself and does not evaluate the streamed
expressions when the level is disabled, so the guard only adds lines.

## Examples

Bad:

```cpp
if (VLOG_IS_ON(2)) {
  VLOG(2) << "Some message";
}
```

Good:

```cpp
VLOG(2) << "Some message";
```
