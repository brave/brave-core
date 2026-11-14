---
name: brave/ui-text-sentence-case
description:
  User-facing strings, including titles, headings and buttons, use sentence
  case; title case is only for branded product and plan names, and where Apple's
  platform rules require it on iOS and macOS.
rubric: readability
severity: should_fix
---

**Rule.** Write user-facing strings in sentence case: capitalize only the first
word, proper nouns and branded names. This includes page titles, section
headers, headlines, button labels, menu items, email subject lines and system
warnings. Use title case only for branded product and plan names ("Brave
Rewards", "Brave Talk Premium"), and where Apple's platform rules require it:
iOS UI copy and settings, and the `use_titlecase` variants of macOS menu items
and similar strings.

**Why.** Sentence case is easier to read and is Brave's default; title case on
generic words makes ordinary features look like branded products.

## Examples

Bad:

```xml
<message name="IDS_SETTINGS_BACKGROUND_TITLE" desc="Title of the background image section">
  Background Image Options
</message>
```

Good:

```xml
<message name="IDS_SETTINGS_BACKGROUND_TITLE" desc="Title of the background image section">
  Background image options
</message>
```
