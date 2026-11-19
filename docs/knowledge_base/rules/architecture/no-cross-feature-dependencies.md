---
name: brave/no-cross-feature-dependencies
description:
  'A feature module does not import classes from an unrelated feature module
  (VPN from Rewards or Wallet); shared code moves to a common utility or behind
  an interface.'
rubric: design
severity: should_fix
---

**Rule.** Code of one feature (VPN, Rewards, Wallet, Playlist, ...) does not
include, import or depend on classes of another, unrelated feature. When both
need the same functionality, extract it into a common utility, or have the
consumer depend on an interface the other implements.

**Why.** Cross-feature dependencies couple features so that neither can be
modified or removed independently.

## Examples

Bad:

```java
// android/java/org/chromium/chrome/browser/vpn/utils/BraveVpnUtils.java
import org.chromium.chrome.browser.BraveRewardsHelper;
```

Good:

```java
// android/java/org/chromium/chrome/browser/vpn/utils/BraveVpnUtils.java
import org.chromium.chrome.browser.util.TabUtils;
```
