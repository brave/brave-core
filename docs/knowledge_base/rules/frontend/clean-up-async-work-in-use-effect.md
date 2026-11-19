---
name: brave/clean-up-async-work-in-use-effect
description:
  Async operations started in useEffect must be cleaned up, with an
  AbortController or a ref-based cancellation flag, so they cannot update state
  after unmount.
rubric: correctness
severity: blocking
---

**Rule.** A `useEffect` that starts an asynchronous operation (a request, a
promise chain, an `await`) MUST return a cleanup function that cancels it: abort
an `AbortController` whose signal the operation takes, or set a ref-based
cancellation flag that the callback checks before it updates state.

**Why.** Without cleanup, the operation completes after the component has
unmounted and updates the state of an unmounted component.

## Examples

Bad:

```tsx
useEffect(() => {
  fetchData().then(setData)
}, [])
```

Good:

```tsx
useEffect(() => {
  const controller = new AbortController()
  fetchData({ signal: controller.signal }).then(setData)
  return () => controller.abort()
}, [])
```
