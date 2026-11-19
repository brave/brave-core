---
name: brave/ui-text-phone-or-tablet
description:
  User-facing text calls mobile devices "phone or tablet" on first reference and
  "phone", never "smartphone", using device names in subheads.
rubric: readability
severity: nit
---

**Rule.** On first reference, call a mobile device a "phone or tablet", not a
"device" or "mobile device", and never a "smartphone" ("phone" instead). Use
actual device names (iPhone, iPad, Android) in subheads. Do not repeat "phone or
tablet" more than once in a paragraph: after it, say "your device".

**Why.** Concrete words are clearer than "device", and the rules keep the text
from repeating itself.

## Examples

Bad:

```xml
<message name="IDS_SYNC_MOBILE_HINT" desc="Hint in the sync setup page">
  Scan this code with your smartphone or mobile device.
</message>
```

Good:

```xml
<message name="IDS_SYNC_MOBILE_HINT" desc="Hint in the sync setup page">
  Scan this code with your phone or tablet.
</message>
```
