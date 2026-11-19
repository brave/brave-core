---
name: brave/ui-text-avoid-repeating-your
description: A user-facing string does not repeat "your" where one is enough.
rubric: readability
severity: nit
---

**Rule.** Do not repeat "your" within a string. Keep the first and drop the
rest, rephrasing if needed: "Log in to your Premium account to manage
subscriptions", not "...to manage your subscription".

**Why.** Repeated possessives make strings longer and heavier to read without
adding meaning.

## Examples

Bad:

```xml
<message name="IDS_PREMIUM_LOGIN" desc="Body text of the Premium login prompt">
  Log in to your Premium account to manage your subscription.
</message>
```

Good:

```xml
<message name="IDS_PREMIUM_LOGIN" desc="Body text of the Premium login prompt">
  Log in to your Premium account to manage subscriptions.
</message>
```
