---
name: brave/ui-allow-for-text-expansion
description:
  UI layouts allow for translated text being longer than English, rather than
  fixing sizes or truncating to fit the English string.
rubric: design
severity: should_fix
---

**Rule.** Lay out text so it can grow when translated: let labels wrap or the
container grow, and avoid fixed widths, single-line limits or truncation sized
to the English string.

**Why.** Translations are often much longer than English. A layout sized to the
English text clips or overflows in other languages.

## Examples

Bad:

```css
.tab-label {
  width: 80px;
  white-space: nowrap;
  overflow: hidden;
}
```

Good:

```css
.tab-label {
  min-width: 80px;
  overflow-wrap: anywhere;
}
```
