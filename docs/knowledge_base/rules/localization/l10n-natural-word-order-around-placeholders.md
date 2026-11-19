---
name: brave/l10n-natural-word-order-around-placeholders
description:
  Strings with placeholders read naturally once an example value is substituted.
rubric: readability
severity: should_fix
---

**Rule.** Write strings with placeholders so the sentence reads naturally with a
real value in place. Check by substituting the example value and reading the
result aloud.

**Why.** Awkward word order around a placeholder reads as machine-generated, and
translators copy it.

## Examples

Bad:

```xml
<message name="IDS_TABS_FROM_DEVICE" desc="Header of the tabs synced from another device">
  Tabs from device <ph name="DEVICE_NAME">$1<ex>Pixel 8</ex></ph> recently opened
</message>
```

Good:

```xml
<message name="IDS_TABS_FROM_DEVICE" desc="Header of the tabs synced from another device">
  Recently opened on <ph name="DEVICE_NAME">$1<ex>Pixel 8</ex></ph>
</message>
```
