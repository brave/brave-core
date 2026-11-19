---
name: brave/ui-text-time-format
description:
  'Times in user-facing text are written as "6:00 am" or "12:15 pm": lowercase
  am/pm after a space.'
rubric: readability
severity: nit
---

**Rule.** Write times as hours and minutes followed by a space and lowercase
"am" or "pm": "6:00 am", "12:15 pm". Not "6AM", "6:00 A.M." or "6:00am".

**Why.** One consistent time format reads cleanly and is simple for translators
to localize.

## Examples

Bad:

```xml
<message name="IDS_SCHEDULED_SCAN" desc="Time of the scheduled scan">
  Scans run daily at 6:00AM.
</message>
```

Good:

```xml
<message name="IDS_SCHEDULED_SCAN" desc="Time of the scheduled scan">
  Scans run daily at 6:00 am.
</message>
```
