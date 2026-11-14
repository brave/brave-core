---
name: brave/l10n-consistent-capitalization
description:
  All strings of one feature or UI surface at the same hierarchy level use the
  same capitalization style.
rubric: readability
severity: should_fix
---

**Rule.** Within a feature or UI surface, strings at the same level (the options
of one menu, the labels of one settings group) MUST use one capitalization
style. Do not mix title case ("Background Image") and sentence case ("Brave
backgrounds") among them; when adding a string, match its siblings.

**Why.** Mixed capitalization among sibling controls looks unfinished, and makes
some read as branded names and others not.

## Examples

Bad:

```xml
<message name="IDS_NTP_BACKGROUND_IMAGE" desc="Option in the background picker">
  Background Image
</message>
<message name="IDS_NTP_BRAVE_BACKGROUNDS" desc="Option in the background picker">
  Brave backgrounds
</message>
```

Good:

```xml
<message name="IDS_NTP_BACKGROUND_IMAGE" desc="Option in the background picker">
  Background image
</message>
<message name="IDS_NTP_BRAVE_BACKGROUNDS" desc="Option in the background picker">
  Brave backgrounds
</message>
```
