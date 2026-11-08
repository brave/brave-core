---
name: brave/ui-text-browser-messaging-claims
description:
  Copy about the Brave browser must not say it blocks all ads and trackers; its
  approved messages are speed, Chrome extension support and 60-second import.
rubric: correctness
severity: should_fix
---

**Rule.** In copy about the browser, NEVER say that Brave blocks all ads and
trackers. The approved messages are speed (up to 3x faster than Chrome), Chrome
extension support and 60-second import.

**Why.** Brave blocks most ads and trackers, not all; an absolute claim is
inaccurate and cannot be backed up.

## Examples

Bad:

```xml
<message name="IDS_WELCOME_SHIELDS" desc="Shields step of the welcome tour">
  Brave blocks all ads and trackers on every site.
</message>
```

Good:

```xml
<message name="IDS_WELCOME_SHIELDS" desc="Shields step of the welcome tour">
  Brave Shields block trackers and ads that follow you across the web.
</message>
```
