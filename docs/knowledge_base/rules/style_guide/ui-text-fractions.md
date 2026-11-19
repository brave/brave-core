---
name: brave/ui-text-fractions
description:
  User-facing text always spells out "half" and writes other fractions as x/y
  without ordinals.
rubric: readability
severity: nit
---

**Rule.** Always spell out "half". Write other fractions as digits with a slash
("1/3", "3/4"), not with ordinals ("one-third", "three-fourths").

**Why.** It keeps fractions short and consistent, and ordinals do not translate
well.

## Examples

Bad:

```xml
<message name="IDS_STORAGE_USED" desc="Storage usage summary">
  You've used 1/2 of your storage and one-third of your quota.
</message>
```

Good:

```xml
<message name="IDS_STORAGE_USED" desc="Storage usage summary">
  You've used half of your storage and 1/3 of your quota.
</message>
```
