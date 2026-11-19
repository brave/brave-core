---
name: brave/ui-text-vpn-streaming-claims
description:
  Copy about Brave VPN must not say it can be used to watch streaming sites.
rubric: correctness
severity: should_fix
---

**Rule.** Do not say Brave VPN can be used to watch streaming sites. The
approved messages are accessing content on the go, free for 7 days, and
availability on mobile and desktop.

**Why.** Streaming sites can restrict VPN traffic, so the promise may not hold,
and users who rely on it are misled.

## Examples

Bad:

```xml
<message name="IDS_VPN_PROMO_BODY" desc="Body of the VPN promotion">
  Unlock streaming sites from anywhere with Brave VPN.
</message>
```

Good:

```xml
<message name="IDS_VPN_PROMO_BODY" desc="Body of the VPN promotion">
  Access content on the go. Free for 7 days.
</message>
```
