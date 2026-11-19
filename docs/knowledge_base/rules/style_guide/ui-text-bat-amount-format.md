---
name: brave/ui-text-bat-amount-format
description:
  BAT amounts in user-facing text are a number followed by "BAT", with balances
  to three decimal places, such as "80.000 BAT".
rubric: readability
severity: nit
---

**Rule.** Write a BAT amount as the number followed by "BAT" ("1 BAT"), and
format BAT balances to three decimal places ("80.000 BAT").

**Why.** A fixed format makes amounts comparable and unambiguous.

## Examples

Bad:

```xml
<message name="IDS_REWARDS_BALANCE" desc="Rewards balance">
  BAT balance: 80.5
</message>
```

Good:

```xml
<message name="IDS_REWARDS_BALANCE" desc="Rewards balance">
  Balance: <ph name="BALANCE">$1<ex>80.500</ex></ph> BAT
</message>
```
