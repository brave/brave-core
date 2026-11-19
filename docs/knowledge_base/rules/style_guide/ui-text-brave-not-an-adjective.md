---
name: brave/ui-text-brave-not-an-adjective
description:
  User-facing text must never use "Brave" or "brave" as an adjective, as in "be
  brave".
rubric: readability
severity: should_fix
---

**Rule.** NEVER use "Brave" or "brave" as an adjective: no "be brave", "be
Brave", "stay brave" or similar wordplay on the brand name.

**Why.** Wordplay on the brand reads as a gimmick and blurs the product name,
and it does not survive translation.

## Examples

Bad:

```xml
<message name="IDS_ONBOARDING_TITLE" desc="Title of the first onboarding step">
  Be Brave online
</message>
```

Good:

```xml
<message name="IDS_ONBOARDING_TITLE" desc="Title of the first onboarding step">
  Welcome to the Brave browser
</message>
```
