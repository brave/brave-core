---
name: brave/ui-text-per-month-per-year
description:
  Subscription terms in user-facing text are "per month" and "per year", not
  "monthly" or "annually".
rubric: readability
severity: nit
---

**Rule.** For subscriptions, write "per month" and "per year", not "monthly",
"annually" or abbreviations such as "/mo".

**Why.** "Per month" states the billing period unambiguously, next to a price
and in translation.

## Examples

Bad:

```xml
<message name="IDS_VPN_PLAN" desc="VPN plan description">
  $9.99 billed monthly
</message>
```

Good:

```xml
<message name="IDS_VPN_PLAN" desc="VPN plan description">
  $9.99 per month
</message>
```
