---
name: brave/grd-placeholder-examples
description:
  GRD/GRDP placeholders for runtime values include an <ex> tag showing
  translators an example value.
rubric: readability
severity: should_fix
---

**Rule.** Include an `<ex>` tag in every `<ph>` that stands for a runtime value,
with a realistic example of that value (`<ex>example.com</ex>`).

**Why.** Without an example, translators cannot tell whether a placeholder is a
name, a number or a URL, and so how the sentence around it must change.

## Examples

Bad:

```xml
<message name="IDS_SHRED_CONFIRMATION" desc="Confirmation dialog for shredding site data">
  Shredding will delete site data and close all <ph name="SITE_NAME">$1</ph> tabs.
</message>
```

Good:

```xml
<message name="IDS_SHRED_CONFIRMATION" desc="Confirmation dialog for shredding site data">
  Shredding will delete site data and close all <ph name="SITE_NAME">$1<ex>example.com</ex></ph> tabs.
</message>
```
