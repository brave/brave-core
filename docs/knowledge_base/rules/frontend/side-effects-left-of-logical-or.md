---
name: brave/side-effects-left-of-logical-or
description:
  Never replace |= with ||=, or write x = x || f(), when the right-hand side has
  side effects; put the side-effecting call on the left of ||.
rubric: correctness
severity: blocking
---

**Rule.** When accumulating a result from a call that has side effects, NEVER
write `flag ||= update()` or `flag = flag || update()`, and never turn a
`flag |= update()` into either. Put the call on the LEFT side of `||`:
`flag = update() || flag`.

**Why.** `||` and `||=` short-circuit: once the left side is truthy, the right
side is never evaluated. The call's side effects are then silently skipped for
every item after the first one that sets the flag.

## Examples

Bad:

```typescript
isDirty ||= updateFileTimestamps(file)
isDirty = isDirty || updateFileTimestamps(file)
```

Good:

```typescript
isDirty = updateFileTimestamps(file) || isDirty
```
