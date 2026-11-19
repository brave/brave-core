---
name: brave/ui-text-bat-first-use
description:
  User-facing text spells out "Basic Attention Token (BAT)" on first usage, then
  uses "BAT".
rubric: readability
severity: should_fix
---

**Rule.** Write "Basic Attention Token (BAT)" the first time a surface mentions
it, and "BAT" after that.

**Why.** New users do not know the acronym; spelling it out once explains it
without repeating the long form.

## Examples

Bad:

```xml
<message name="IDS_REWARDS_INTRO" desc="First paragraph of the Rewards onboarding">
  Support creators with BAT.
</message>
```

Good:

```xml
<message name="IDS_REWARDS_INTRO" desc="First paragraph of the Rewards onboarding">
  Support creators with Basic Attention Token (BAT).
</message>
```
