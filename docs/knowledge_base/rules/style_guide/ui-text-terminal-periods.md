---
name: brave/ui-text-terminal-periods
description:
  Body copy and numbered list items end with a period; titles, headings,
  buttons, bullet points and single-sentence toggle descriptions do not.
rubric: readability
severity: nit
---

**Rule.** End body copy and numbered list items with a period. Do not end
titles, headings, subheads, buttons or bullet points with one, unless an item of
the bullet list has more than one sentence: then every item gets a period. A
settings toggle description gets no period when it is one sentence, and a period
after its last sentence when it has several.

**Why.** Consistent terminal punctuation keeps a surface tidy, and translators
copy the English source's punctuation.

## Examples

Bad:

```xml
<message name="IDS_SETTINGS_AUTOPLAY_TITLE" desc="Title of the autoplay setting">
  Autoplay.
</message>
<message name="IDS_SETTINGS_AUTOPLAY_DESC" desc="Description under the autoplay toggle">
  Block videos from playing automatically.
</message>
```

Good:

```xml
<message name="IDS_SETTINGS_AUTOPLAY_TITLE" desc="Title of the autoplay setting">
  Autoplay
</message>
<message name="IDS_SETTINGS_AUTOPLAY_DESC" desc="Description under the autoplay toggle">
  Block videos from playing automatically
</message>
```
