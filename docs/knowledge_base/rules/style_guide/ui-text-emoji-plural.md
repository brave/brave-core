---
name: brave/ui-text-emoji-plural
description: User-facing text uses "emoji" as the plural, never "emojis".
rubric: readability
severity: nit
---

**Rule.** Use "emoji" for both singular and plural, NEVER "emojis".

**Why.** It is the convention Brave's copy follows, and one form keeps strings
consistent.

## Examples

Bad:

```xml
<message name="IDS_CHAT_REACTIONS" desc="Label of the reactions picker">
  Add emojis
</message>
```

Good:

```xml
<message name="IDS_CHAT_REACTIONS" desc="Label of the reactions picker">
  Add emoji
</message>
```
