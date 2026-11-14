---
name: brave/no-fallbacks-for-leo-css-variables
description:
  Never give a Leo design system CSS custom property a fallback value in var().
rubric: correctness
severity: blocking
---

**Rule.** NEVER pass a fallback to `var()` for a Leo custom property (any
`--leo-*` variable): write `var(--leo-color-text-primary)`, not
`var(--leo-color-text-primary, #000)`. This holds in CSS files, in
styled-components and in inline styles alike.

**Why.** Leo variables are guaranteed to be set by the design system. A fallback
can only take effect when something is wrong, such as a misspelled or removed
token, and then it hides the bug behind a hardcoded value that ignores the
theme, producing inconsistent styling.

## Examples

Bad:

```css
color: var(--leo-color-text-primary, #000);
```

Good:

```css
color: var(--leo-color-text-primary);
```
