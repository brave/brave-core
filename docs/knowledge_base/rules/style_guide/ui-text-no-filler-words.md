---
name: brave/ui-text-no-filler-words
description:
  User-facing strings must not use overused filler words such as "basically",
  "obviously", "literally" or "frankly".
rubric: readability
severity: nit
---

**Rule.** Remove filler words such as "basically", "obviously", "literally" and
"frankly" from UI strings. The sentence says the same without them.

**Why.** Filler words add length without meaning, and "obviously" can make a
reader who did not know feel talked down to.

## Examples

Bad:

```xml
<message name="IDS_SYNC_HINT" desc="Hint text in the sync settings page">
  Sync basically keeps your bookmarks the same on every device.
</message>
```

Good:

```xml
<message name="IDS_SYNC_HINT" desc="Hint text in the sync settings page">
  Sync keeps your bookmarks the same on every device.
</message>
```
