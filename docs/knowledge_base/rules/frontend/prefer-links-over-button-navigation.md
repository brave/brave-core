---
name: brave/prefer-links-over-button-navigation
description:
  When an element's only action is navigating to a URL, use an <a> link, or the
  design system's component, rather than a <button> with JavaScript navigation.
rubric: design
severity: nit
---

**Rule.** If the only thing an element does is navigate to a URL, render it as a
link (`<a href={url}>`), not as a `<button>` whose `onClick` calls
`window.open()` or sets `location`. Exception: where the design system
(Nala/Leo) provides styled button or link components, use those for visual
consistency, even when that means a Leo `Button` that navigates; design system
consistency takes precedence over raw semantic HTML.

**Why.** A link is more accessible: users can see the destination on hover, and
it follows semantic HTML, so assistive technology announces it as the navigation
it is.

## Examples

Bad:

```tsx
<button onClick={() => window.open(url)}>Visit</button>
```

Good:

```tsx
<a href={url} target="_blank" rel="noopener">Visit</a>

// Also correct, where the design system requires its Button.
<Button onClick={() => api.openTab(url)}>Settings</Button>
```
