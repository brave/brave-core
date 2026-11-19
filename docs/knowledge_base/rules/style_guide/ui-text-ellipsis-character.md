---
name: brave/ui-text-ellipsis-character
description:
  User-facing strings use the Unicode ellipsis character (…), never three
  periods.
rubric: readability
severity: should_fix
---

**Rule.** Write the ellipsis as the single Unicode character `…` (U+2026,
option-semicolon on Mac), NEVER as three periods `...`.

**Why.** The ellipsis character is the standard typographic convention for UI
text. Three periods can wrap across lines and are spaced differently from the
real character.

## Examples

Bad:

```xml
<message name="IDS_SAVE_AS_MENU_ITEM" desc="Menu item that opens the save dialog">
  Save as...
</message>
```

Good:

```xml
<message name="IDS_SAVE_AS_MENU_ITEM" desc="Menu item that opens the save dialog">
  Save as…
</message>
```
