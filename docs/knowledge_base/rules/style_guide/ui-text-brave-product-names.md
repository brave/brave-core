---
name: brave/ui-text-brave-product-names
description:
  Brave products and features are named exactly as listed (title case) on first
  use or for the branded feature, with lowercase allowed only for general usage.
rubric: readability
severity: should_fix
---

**Rule.** Name Brave products and features exactly, in title case: Basic
Attention Token, Brave Ads / Brave Private Ads, Brave browser, Brave Firewall +
VPN, Brave Leo AI, Brave News, Brave Playlist, Brave Premium, Brave Rewards,
Brave Search, Brave Shields, Brave Swap, Brave Talk / Brave Talk Premium, Brave
Wallet, Brave Creators, and Ad-free Brave Search ("ad-free Brave Search"
mid-sentence). Use the full name on first use and whenever the text means the
branded feature; lowercase is fine for general usage ("create a playlist",
"manage your rewards"). NEVER write "Brave ads" or "private ads" for the
product.

**Why.** Consistent product names are part of the brand, and translators need to
know which words are names that stay untranslated.

## Examples

Bad:

```xml
<message name="IDS_ADS_OPT_IN" desc="Opt-in prompt for ads">
  Turn on brave private ads to earn BAT.
</message>
```

Good:

```xml
<message name="IDS_ADS_OPT_IN" desc="Opt-in prompt for ads">
  Turn on Brave Private Ads to support creators with BAT.
</message>
```
