---
name: brave/catch-json-parse-errors-in-react-render
description:
  JSON.parse of external or server data in a React render path, including
  useMemo, must be wrapped in try/catch.
rubric: correctness
severity: blocking
---

**Rule.** Every `JSON.parse()` of external or server data that runs while a
React component renders (in the component body, or in a `useMemo` or other hook
it calls) MUST be wrapped in `try`/`catch`, falling back to a default when
parsing fails.

**Why.** Malformed JSON throws a `SyntaxError`, and an exception thrown while
rendering crashes the entire component tree when nothing catches it.

## Examples

Bad:

```tsx
const config = useMemo(() => JSON.parse(rawData), [rawData])
```

Good:

```tsx
const config = useMemo(() => {
  try {
    return JSON.parse(rawData)
  } catch {
    return defaultConfig
  }
}, [rawData])
```
