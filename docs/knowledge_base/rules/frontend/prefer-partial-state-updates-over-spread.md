---
name: brave/prefer-partial-state-updates-over-spread
description:
  To change some fields of a state object, use a partial update function rather
  than spreading the whole object and overriding them.
rubric: readability
severity: nit
---

**Rule.** When updating some fields of a state object, use a partial update
function (such as `setPartialState()`) with only the changed fields
(`setPartialState({ isLoading: true })`), instead of spreading the whole object
and overriding them (`setState({ ...state, isLoading: true })`).

**Why.** A partial update is more efficient and less error-prone: spreading
copies every field, and spreading a `state` captured earlier writes back values
that may have changed since it was read.

## Examples

Bad:

```tsx
setState({ ...state, isLoading: true })
```

Good:

```tsx
setPartialState({ isLoading: true })
```
