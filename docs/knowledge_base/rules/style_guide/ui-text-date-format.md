---
name: brave/ui-text-date-format
description:
  Dates in user-facing text are written without ordinal numbers or numeric
  formats, such as "June 1" rather than "June 1st" or "6/1/21".
rubric: readability
severity: nit
---

**Rule.** Write dates with the month name and a plain number: "June 1". Do not
use ordinals ("June 1st") or numeric formats ("6/1/21").

**Why.** Numeric dates are read differently by locale (day/month or month/day),
and ordinals do not translate.

## Examples

Bad:

```xml
<message name="IDS_TRIAL_ENDS" desc="Notice of the trial end date">
  Your trial ends on June 1st (6/1/21).
</message>
```

Good:

```xml
<message name="IDS_TRIAL_ENDS" desc="Notice of the trial end date">
  Your trial ends on June 1.
</message>
```
