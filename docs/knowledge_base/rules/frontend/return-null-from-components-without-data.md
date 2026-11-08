---
name: brave/return-null-from-components-without-data
description:
  A React component with no data to render returns null early, rather than
  rendering an empty container or placeholder markup.
rubric: perf
severity: nit
---

**Rule.** When a component has nothing to show, `return null` before building
any markup. Do not render its container with content that falls back to `''`, or
placeholder elements that stay empty.

**Why.** An empty container or placeholder markup adds unnecessary DOM nodes
that show the user nothing. An early `return null` adds none, and says plainly
that the component renders nothing without its data.

## Examples

Bad:

```tsx
function UserInfo({ user }: Props) {
  return <div className='user-info'>{user ? user.name : ''}</div>
}
```

Good:

```tsx
function UserInfo({ user }: Props) {
  if (!user) return null
  return <div className='user-info'>{user.name}</div>
}
```
