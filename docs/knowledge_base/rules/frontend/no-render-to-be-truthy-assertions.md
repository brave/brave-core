---
name: brave/no-render-to-be-truthy-assertions
description:
  React tests never assert that the result of render() is truthy, which always
  passes; they assert on the rendered content.
rubric: testing
severity: should_fix
---

**Rule.** Do NOT assert `expect(render(<Component />)).toBeTruthy()`, or the
same of the object `render()` returned. Query the rendered output (`getByText`,
`getByRole`) and assert on what it finds.

**Why.** `render()` always returns a truthy object, so the assertion always
passes and the test checks nothing, not even that the component rendered what it
should.

## Examples

Bad:

```tsx
expect(render(<MyComponent />)).toBeTruthy()
```

Good:

```tsx
const { getByText } = render(<MyComponent />)
expect(getByText('Expected text')).toBeTruthy()
```
