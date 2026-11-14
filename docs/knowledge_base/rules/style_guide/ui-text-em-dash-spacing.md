---
name: brave/ui-text-em-dash-spacing
description:
  User-facing text uses the em dash (—) with no space before or after it.
rubric: readability
severity: nit
---

**Rule.** Write a dash as the em dash character `—` (option-shift-hyphen), with
no space on either side. Do not use a spaced hyphen or a double hyphen in its
place.

**Why.** A consistent dash style keeps text tidy, and spaced hyphens can be
mistaken for minus signs or break at line ends.

## Examples

Bad:

```xml
<message name="IDS_VPN_PROMO" desc="Promotional text for Brave VPN">
  Brave VPN - free for 7 days
</message>
```

Good:

```xml
<message name="IDS_VPN_PROMO" desc="Promotional text for Brave VPN">
  Brave VPN—free for 7 days
</message>
```
