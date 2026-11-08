---
name: brave/no-use-memo-for-trivial-access
description:
  Do not wrap a simple property lookup or trivial derivation such as
  items.length in useMemo; compute it directly.
rubric: perf
severity: nit
---

**Rule.** Do not use `useMemo` for a simple property lookup or other trivial
derivation (`array.length`, `obj.property`, a comparison of them). Compute the
value directly in the render.

**Why.** `useMemo` stores its dependencies and compares them on every render,
which costs more than the property access it is meant to save, and the extra
wrapper makes trivial code look expensive.

## Examples

Bad:

```tsx
const count = useMemo(() => items.length, [items])
```

Good:

```tsx
const count = items.length
```
