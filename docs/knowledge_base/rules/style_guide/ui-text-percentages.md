---
name: brave/ui-text-percentages
description:
  Percentages in user-facing text are digits followed by % with no space, such
  as "50%".
rubric: readability
severity: nit
---

**Rule.** Write percentages as digits immediately followed by `%`, with no space
and no "percent": "50%".

**Why.** One consistent format is quicker to read and gives translators a clear
source to localize.

## Examples

Bad:

```xml
<message name="IDS_DOWNLOAD_PROGRESS" desc="Download progress label">
  <ph name="PERCENT">$1<ex>50</ex></ph> % downloaded
</message>
```

Good:

```xml
<message name="IDS_DOWNLOAD_PROGRESS" desc="Download progress label">
  <ph name="PERCENT">$1<ex>50</ex></ph>% downloaded
</message>
```
