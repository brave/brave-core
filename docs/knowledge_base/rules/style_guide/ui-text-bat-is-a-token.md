---
name: brave/ui-text-bat-is-a-token
description:
  User-facing text calls BAT a "token" or "crypto asset", never a "point",
  "currency" or "cryptocurrency".
rubric: readability
severity: should_fix
---

**Rule.** Call BAT a "token" or a "crypto asset". NEVER call it a "point", a
"currency" or a "cryptocurrency".

**Why.** BAT is a token; calling it a point, a currency or a cryptocurrency
misdescribes what users hold.

## Examples

Bad:

```xml
<message name="IDS_REWARDS_BALANCE_HELP" desc="Help text for the Rewards balance">
  BAT is the cryptocurrency you get for viewing ads.
</message>
```

Good:

```xml
<message name="IDS_REWARDS_BALANCE_HELP" desc="Help text for the Rewards balance">
  BAT is the token you get for viewing Brave Ads.
</message>
```
