---
name: brave/ui-text-single-space-after-period
description: User-facing text has one space, never two, after a period.
rubric: readability
severity: nit
---

**Rule.** Put exactly one space after a period, never two.

**Why.** Double spaces render as uneven gaps and are copied into translations.

## Examples

Bad:

```xml
<message name="IDS_RESTART_NOTICE" desc="Notice shown after an update">
  Brave updated.  Restart to finish.
</message>
```

Good:

```xml
<message name="IDS_RESTART_NOTICE" desc="Notice shown after an update">
  Brave updated. Restart to finish.
</message>
```
