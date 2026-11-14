---
name: brave/check-usages-before-changing-shared-resources
description:
  A change to a shared Android resource value (color, dimension or style, as in
  brave_colors.xml) is checked against every UI that references it.
rubric: correctness
severity: should_fix
---

**Rule.** Before an existing shared resource value (a color, dimension or style
in a file such as `brave_colors.xml`) is modified, every usage of that resource
MUST be cross-checked, and the change verified on ALL the screens that reference
it. When only one screen should change, give it a new resource instead.

**Why.** A shared resource is used by several screens; a value changed to fix
one of them silently changes the others too.

## Examples

Bad:

```xml
<!-- brave_colors.xml: darkened for the wallet header, also used by settings. -->
<color name="brave_theme_color">#1A1C3B</color>
```

Good:

```xml
<!-- A new color for the one screen that needs it. -->
<color name="wallet_header_color">#1A1C3B</color>
```
