---
name: brave/ui-text-turn-on-not-enable
description:
  User-facing text says "turn on" and "turn off" instead of "enable" and
  "disable".
rubric: readability
severity: should_fix
---

**Rule.** Write "turn on" and "turn off", not "enable" and "disable" (nor
"enabled" and "disabled").

**Why.** "Turn on" is plainer language and matches what users do with a toggle.

## Examples

Bad:

```xml
<message name="IDS_SETTINGS_SYNC_DISABLED" desc="Status of the sync setting">
  Enable sync to keep your bookmarks on every device.
</message>
```

Good:

```xml
<message name="IDS_SETTINGS_SYNC_OFF" desc="Status of the sync setting">
  Turn on sync to keep your bookmarks on every device.
</message>
```
