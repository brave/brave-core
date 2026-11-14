---
name: brave/ui-text-shields-capitalized
description:
  '"Shields" is always capitalized in user-facing text, even in general usage.'
rubric: readability
severity: should_fix
---

**Rule.** Always capitalize "Shields", even in general usage where other feature
names may be lowercase ("put Shields down", "Shields settings").

**Why.** Shields is Brave's signature feature and is treated as a proper noun
everywhere.

## Examples

Bad:

```xml
<message name="IDS_SHIELDS_HINT" desc="Hint in the site settings panel">
  Turn shields down if this site isn't working.
</message>
```

Good:

```xml
<message name="IDS_SHIELDS_HINT" desc="Hint in the site settings panel">
  Put Shields down if this site isn't working.
</message>
```
