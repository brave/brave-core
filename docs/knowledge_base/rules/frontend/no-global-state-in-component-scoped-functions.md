---
name: brave/no-global-state-in-component-scoped-functions
description:
  A function serving one component instance must not rely on global state such
  as window.getSelection() that every instance shares; pass it a ref to its
  instance.
rubric: correctness
severity: should_fix
---

**Rule.** When a function reads global state (`window.getSelection()`, a
`document`-wide query), check what it does when several instances of the
component are on the page. If the operation belongs to one instance, pass the
function a ref to that instance's element and work within it, instead of relying
on the global state.

**Why.** Global state is shared by every instance: the selection may be in
another instance, or a query finds another instance's element, so the function
acts on the wrong one.

## Examples

Bad:

```tsx
function getSelectedText() {
  return window.getSelection()?.toString() ?? ''
}
```

Good:

```tsx
function getSelectedText(container: HTMLElement) {
  const selection = window.getSelection()
  if (!selection || !container.contains(selection.anchorNode)) {
    return ''
  }
  return selection.toString()
}

// In the component:
const ref = React.useRef<HTMLDivElement>(null)
const onCopy = () => ref.current && copy(getSelectedText(ref.current))
```
