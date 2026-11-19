---
name: brave/no-unnecessary-wait-for-in-react-tests
description:
  React component tests do not use waitFor for updates made with rerender or
  inside act(), which are applied synchronously.
rubric: testing
severity: should_fix
---

**Rule.** After updating a component with `rerender(...)`, assert on the result
directly: `rerender` is synchronous. Likewise after DOM mutations wrapped in
`act()`, which applies the pending React updates. Do not wrap those assertions
in `waitFor`.

**Why.** `waitFor` retries its callback until it passes or times out. Where the
update is already applied it only makes the test slower, and it can mask
timing-related bugs, such as an update that should be immediate but arrives
late.

## Examples

Bad:

```tsx
const { getByText, rerender } = render(<Counter count={1} />)
rerender(<Counter count={2} />)
await waitFor(() => expect(getByText('2')).toBeTruthy())
```

Good:

```tsx
const { getByText, rerender } = render(<Counter count={1} />)
rerender(<Counter count={2} />)
expect(getByText('2')).toBeTruthy()
```
