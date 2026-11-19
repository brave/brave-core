---
name: brave/ui-text-en-dash-for-ranges
description:
  Date and time spans in user-facing text use an en dash (–) in place of "to",
  such as "Monday–Friday".
rubric: readability
severity: nit
---

**Rule.** Write a date or time span with an en dash `–` replacing "to":
"Monday–Friday", "9:00 am–5:00 pm". Do not use a hyphen.

**Why.** The en dash is the typographic convention for spans and keeps the range
compact.

## Examples

Bad:

```xml
<message name="IDS_SUPPORT_HOURS" desc="Support availability text">
  Support is available Monday - Friday.
</message>
```

Good:

```xml
<message name="IDS_SUPPORT_HOURS" desc="Support availability text">
  Support is available Monday–Friday.
</message>
```
