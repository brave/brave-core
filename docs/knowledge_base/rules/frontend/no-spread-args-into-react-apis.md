---
name: brave/no-spread-args-into-react-apis
description:
  Do not spread arbitrary arguments or props into render(), a component's props
  or other React APIs; pass explicit props and arguments.
rubric: security
severity: should_fix
---

**Rule.** Do not spread an arbitrary argument list or props object into
`render()`, into a component's props (`<Foo {...props} />`), or into other React
APIs such as `React.createElement()`. Name each prop and argument explicitly. A
test helper wrapping `render()` takes `ui: React.ReactElement` and
`options?: RenderOptions`, not `...args: Parameters<typeof render>`.

**Why.** Whatever an arbitrary spread carries reaches the element, so attributes
its author never intended can be injected, which can lead to XSS. Explicit props
show at the call site exactly what is passed.

## Examples

Bad:

```tsx
async function renderMyComponent(
  ...args: Parameters<typeof render>
): Promise<ReturnType<typeof render>> {
  let result: ReturnType<typeof render>
  await act(async () => {
    result = render(...args)
  })
  return result!
}
```

Good:

```tsx
async function renderMyComponent(
  ui: React.ReactElement,
  options?: RenderOptions,
): Promise<RenderResult> {
  let result: RenderResult
  await act(async () => {
    result = render(ui, options)
  })
  return result!
}
```
