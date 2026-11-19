---
name: brave/ui-text-computer-for-desktop
description:
  User-facing text calls non-mobile machines a "computer", not a "laptop", "PC"
  or "desktop".
rubric: readability
severity: nit
---

**Rule.** Say "computer" for non-mobile machines. Do not say "laptop", "PC" or
"desktop", which leave out users on other form factors.

**Why.** "Computer" covers every form factor Brave runs on.

## Examples

Bad:

```xml
<message name="IDS_SYNC_DESKTOP_HINT" desc="Hint in the mobile sync setup">
  Open Brave on your PC to finish setup.
</message>
```

Good:

```xml
<message name="IDS_SYNC_DESKTOP_HINT" desc="Hint in the mobile sync setup">
  Open the Brave browser on your computer to finish setup.
</message>
```
