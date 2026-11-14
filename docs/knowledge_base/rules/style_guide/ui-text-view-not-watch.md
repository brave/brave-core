---
name: brave/ui-text-view-not-watch
description:
  User-facing text says "view" for content, rather than "watch" or "see".
rubric: readability
severity: nit
---

**Rule.** When referring to content (pages, videos, media, details), say "view",
not "watch" or "see".

**Why.** "View" is the consistent term across Brave's surfaces and applies to
all kinds of content.

## Examples

Bad:

```xml
<message name="IDS_PLAYLIST_OPEN" desc="Menu item that opens a playlist item">
  Watch now
</message>
```

Good:

```xml
<message name="IDS_PLAYLIST_OPEN" desc="Menu item that opens a playlist item">
  View now
</message>
```
