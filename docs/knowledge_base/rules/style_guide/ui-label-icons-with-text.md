---
name: brave/ui-label-icons-with-text
description:
  Icons in the UI have text labels, which make them more translatable and
  accessible.
rubric: design
severity: should_fix
---

**Rule.** Pair icons with a localized text label, visible where space allows,
rather than relying on the icon alone.

**Why.** An icon's meaning differs between cultures, while a label is translated
with the rest of the UI and read by screen readers.

## Examples

Bad:

```tsx
<Button
  fab
  kind='plain-faint'
  onClick={addBookmark}
>
  <Icon name='bookmark' />
</Button>
```

Good:

```tsx
<Button
  kind='plain-faint'
  onClick={addBookmark}
>
  <Icon
    slot='icon-before'
    name='bookmark'
  />
  {getLocale(S.ADD_BOOKMARK)}
</Button>
```
