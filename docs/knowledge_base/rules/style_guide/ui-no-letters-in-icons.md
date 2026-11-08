---
name: brave/ui-no-letters-in-icons
description:
  Icons must not contain letters such as "Aa", which do not translate across
  scripts.
rubric: design
severity: should_fix
---

**Rule.** Do not use letters or characters as an icon (such as "Aa" for text
settings, or "B" for bold), whether drawn in an SVG or written as the text of an
icon element. Use a symbol that does not depend on a script.

**Why.** Letters belong to one script: "Aa" means nothing to a reader of Arabic,
and the icon cannot be translated.

## Examples

Bad:

```tsx
<button className='font-size-button'>Aa</button>
```

Good:

```tsx
<button
  className='font-size-button'
  aria-label={getLocale(S.FONT_SIZE)}
>
  <Icon name='font-size' />
</button>
```
