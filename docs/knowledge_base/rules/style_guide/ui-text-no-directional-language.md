---
name: brave/ui-text-no-directional-language
description:
  User-facing text avoids directional language such as "to the right of",
  "above" or "below", adding a further description when it cannot.
rubric: readability
severity: should_fix
---

**Rule.** Do not locate UI by direction ("to the right of", "above", "below",
"in the top left"). Name the control or describe it instead; when a direction is
unavoidable, add a description that does not depend on it.

**Why.** Directions mean nothing to screen reader users, and layouts change with
screen size and right-to-left languages.

## Examples

Bad:

```xml
<message name="IDS_REWARDS_HINT" desc="Hint in the Rewards onboarding">
  Use the button to the right of the address bar to open Rewards.
</message>
```

Good:

```xml
<message name="IDS_REWARDS_HINT" desc="Hint in the Rewards onboarding">
  Select the Brave Rewards icon in the toolbar to open Rewards.
</message>
```
