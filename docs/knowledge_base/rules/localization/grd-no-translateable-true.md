---
name: brave/grd-no-translateable-true
description:
  GRD/GRDP messages never set translateable="true", which is already the
  default.
rubric: readability
severity: nit
---

**Rule.** NEVER add `translateable="true"` to a `<message>` in a GRD or GRDP
file: messages are translatable by default. Only `translateable="false"`, for
strings excluded from translation, has an effect.

**Why.** The attribute is redundant, and the tooling only looks for
`translateable="false"`, so an explicit `"true"` is noise that suggests it
matters.

## Examples

Bad:

```xml
<message name="IDS_SKILL_SAVED" desc="Toast shown after a skill is saved" translateable="true">
  Skill saved
</message>
```

Good:

```xml
<message name="IDS_SKILL_SAVED" desc="Toast shown after a skill is saved">
  Skill saved
</message>
```
