---
name: brave/ui-text-no-naming-companies-or-activism
description:
  Product copy positions Brave as an alternative to Big Tech and ad tech,
  without calling out specific companies or encouraging activism.
rubric: correctness
severity: should_fix
---

**Rule.** Position Brave as an alternative to "Big Tech" or "ad tech" in
general. Do not call out specific companies (such as Meta) in product messaging,
and do not encourage activism.

**Why.** Brave's product copy positions it as an alternative; naming competitors
or urging activism turns product copy into a campaign.

## Examples

Bad:

```xml
<message name="IDS_NTP_PROMO" desc="Promotion on the new tab page">
  Stop Meta from tracking you. Join the fight for privacy!
</message>
```

Good:

```xml
<message name="IDS_NTP_PROMO" desc="Promotion on the new tab page">
  A private alternative to Big Tech
</message>
```
