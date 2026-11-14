---
name: brave/grd-desc-names-ui-element
description:
  "A string's description says which UI element it is: a title, button label,
  description, tooltip, placeholder, menu item and so on."
rubric: readability
severity: should_fix
---

**Rule.** Say in each description what kind of UI element the string is: a
title, button label, description, tooltip, placeholder, menu item, and so on.
Write `desc="Title for the about section"`, not `desc="About section text"`.

**Why.** The element type decides grammar and length in many languages: a button
takes an imperative, a title a noun phrase, a tooltip a short sentence.

## Examples

Bad:

```xml
<message name="IDS_SETTINGS_ABOUT_TITLE" desc="About section text">
  About Brave
</message>
```

Good:

```xml
<message name="IDS_SETTINGS_ABOUT_TITLE" desc="Title for the about section">
  About Brave
</message>
```
