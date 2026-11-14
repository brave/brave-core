---
name: brave/ui-text-space-before-units
description:
  User-facing text puts a space between a number and its unit of measurement,
  such as "1 TB" or "100 GB".
rubric: readability
severity: should_fix
---

**Rule.** Put a space between a number, or a numeric placeholder, and the unit
that follows it: "1 TB", "100 GB", "5 MB".

**Why.** It is the standard typographic convention, and unit formatting differs
between locales, which a space keeps separable.

## Examples

Bad:

```xml
<message name="IDS_CACHE_SIZE" desc="Size of the cached data">
  <ph name="SIZE">$1<ex>100</ex></ph>MB used
</message>
```

Good:

```xml
<message name="IDS_CACHE_SIZE" desc="Size of the cached data">
  <ph name="SIZE">$1<ex>100</ex></ph> MB used
</message>
```
