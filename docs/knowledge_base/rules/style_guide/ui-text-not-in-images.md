---
name: brave/ui-text-not-in-images
description:
  Images must not contain readable text; text is kept separate from images so it
  can be translated.
rubric: design
severity: should_fix
---

**Rule.** Do not put readable text inside images or SVG artwork (such as a
`<text>` element). Render the text from a localized string on top of or next to
the image.

**Why.** Text inside an image cannot be translated, and screen readers cannot
read it.

## Examples

Bad:

```svg
<svg viewBox="0 0 200 40">
  <text x="10" y="25">Try Brave VPN</text>
</svg>
```

Good:

```svg
<svg viewBox="0 0 200 40">
  <path d="M10 10h180v20H10z" />
</svg>
```
