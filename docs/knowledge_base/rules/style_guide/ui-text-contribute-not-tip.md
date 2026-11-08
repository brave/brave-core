---
name: brave/ui-text-contribute-not-tip
description:
  Giving BAT through Brave Rewards is "contributing" or "supporting", never
  donating, gifting, giving, paying, sending, tipping or earning money.
rubric: readability
severity: should_fix
---

**Rule.** For BAT contributions and Brave Rewards, say "contribute" or "support"
("contributing", "supporting"); for what users receive, say "getting paid". Do
not say "donate", "gift", "give", "pay", "send" or "tip" for contributions, and
NEVER say "tipping" or "earning money".

**Why.** Brave keeps one vocabulary for Rewards. The avoided words misdescribe
BAT contributions as donations, payments or tips, and Rewards as a way to earn
money.

## Examples

Bad:

```xml
<message name="IDS_REWARDS_TIP_BUTTON" desc="Button that opens the contribution panel">
  Tip this creator
</message>
```

Good:

```xml
<message name="IDS_REWARDS_CONTRIBUTE_BUTTON" desc="Button that opens the contribution panel">
  Support this creator
</message>
```
