---
name: brave/ui-text-tor-claims-need-approval
description:
  User-facing text must not make privacy, security or anonymity claims about Tor
  windows without Security/Privacy team approval.
rubric: correctness
severity: blocking
---

**Rule.** Do not add privacy, security or anonymity claims about Tor windows
("untraceable", "hides your identity", "fully anonymous") unless the
Security/Privacy team has approved the wording.

**Why.** Tor's protections have limits that depend on how it is used; overstated
claims can put users at real risk.

## Examples

Bad:

```xml
<message name="IDS_TOR_WINDOW_DESC" desc="Description on the new Tor window page">
  Tor windows make you completely anonymous online.
</message>
```

Good:

```xml
<message name="IDS_TOR_WINDOW_DESC" desc="Description on the new Tor window page">
  Tor windows route your traffic through the Tor network.
</message>
```
