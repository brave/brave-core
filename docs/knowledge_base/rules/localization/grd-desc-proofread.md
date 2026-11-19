---
name: brave/grd-desc-proofread
description: String descriptions are proofread and free of typos.
rubric: readability
severity: nit
---

**Rule.** Proofread each `desc`: fix typos, misspelled feature names and broken
grammar.

**Why.** Typos in descriptions confuse translators and lower translation
quality.

## Examples

Bad:

```xml
<message name="IDS_PLAYLIST_ADD" desc="Buton label that adds the curent video to a playlst">
  Add to playlist
</message>
```

Good:

```xml
<message name="IDS_PLAYLIST_ADD" desc="Button label that adds the current video to a playlist">
  Add to playlist
</message>
```
