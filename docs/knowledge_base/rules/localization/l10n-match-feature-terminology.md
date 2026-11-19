---
name: brave/l10n-match-feature-terminology
description:
  New strings use the feature's established vocabulary, such as Shields being
  "up" or "down" rather than "on" or "off".
rubric: readability
severity: should_fix
---

**Rule.** Before adding strings for a feature, check its existing strings and
reuse their vocabulary. Shields, for example, are "up" or "down", never "on" or
"off": "with Shields up", not "with Shields on".

**Why.** Users learn a feature's words; switching terms makes them wonder
whether the new string means something else.

## Examples

Bad:

```xml
<message name="IDS_SHIELDS_STATUS" desc="Status line in the Shields panel">
  Shields are on for this site
</message>
```

Good:

```xml
<message name="IDS_SHIELDS_STATUS" desc="Status line in the Shields panel">
  Shields are up for this site
</message>
```
