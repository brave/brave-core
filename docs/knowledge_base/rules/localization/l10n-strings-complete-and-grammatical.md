---
name: brave/l10n-strings-complete-and-grammatical
description:
  User-facing strings are complete, grammatically correct sentences with proper
  punctuation and prepositions that fit the UI.
rubric: readability
severity: should_fix
---

**Rule.** Check that each new or changed string is complete (not cut off
mid-thought), grammatically correct and properly punctuated, with prepositions
that fit the context ("in" or "on" a UI location).

**Why.** Translators use the English source as their reference, so its errors
propagate into every translation.

## Examples

Bad:

```xml
<message name="IDS_SIDEBAR_HINT" desc="Hint about the sidebar">
  Find your bookmarks in the sidebar, which you
</message>
```

Good:

```xml
<message name="IDS_SIDEBAR_HINT" desc="Hint about the sidebar">
  Find your bookmarks in the sidebar.
</message>
```
