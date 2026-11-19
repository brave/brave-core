---
name: brave/no-redundant-react-keys
description:
  When a list's parent element already has the key, the elements inside that
  list item must not set their own key for list keying.
rubric: readability
severity: nit
---

**Rule.** In a list rendered with `.map()`, put the `key` only on the element
the callback returns. Do not repeat a `key` on the elements nested inside it for
list-keying purposes: they are not the list's items.

**Why.** React reconciles a list by the keys of its direct items, so a key on an
inner element does nothing for the list. Redundant keys suggest a
misunderstanding of React's reconciliation boundary, and mislead the next reader
about which element the list is keyed by.

## Examples

Bad:

```tsx
{
  items.map((item) => (
    <AttachmentItem key={item.id}>
      <div key={item.id}>{item.name}</div>
    </AttachmentItem>
  ))
}
```

Good:

```tsx
{
  items.map((item) => (
    <AttachmentItem key={item.id}>
      <div>{item.name}</div>
    </AttachmentItem>
  ))
}
```
