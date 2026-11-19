---
name: brave/ui-text-bat-no-cash-value
description:
  User-facing text must not discuss BAT as having a cash value, nor put a
  currency symbol next to BAT.
rubric: readability
severity: should_fix
---

**Rule.** Do not describe BAT as having a cash value, and NEVER accompany a BAT
amount with a currency symbol ($, €).

**Why.** Presenting BAT as cash, or pricing it in a currency, misdescribes what
it is.

## Examples

Bad:

```xml
<message name="IDS_REWARDS_EARNINGS" desc="Summary of Rewards this month">
  You have $5 worth of BAT this month.
</message>
```

Good:

```xml
<message name="IDS_REWARDS_EARNINGS" desc="Summary of Rewards this month">
  You have <ph name="AMOUNT">$1<ex>5.000</ex></ph> BAT this month.
</message>
```
