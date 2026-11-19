---
name: brave/ui-text-no-colloquialisms
description:
  User-facing strings must not use colloquial language, idioms or slang.
rubric: readability
severity: should_fix
---

**Rule.** Do not use colloquial language, idioms or slang in UI strings. Use the
plain word for what is meant: "suspicious", not "creepy" or "sketchy"; "fast",
not "lightning quick".

**Why.** Idioms and slang can confuse or offend readers, and translators often
cannot carry them into other languages.

## Examples

Bad:

```xml
<message name="IDS_AD_REPORT_REASON" desc="Option in the ad report menu">
  This ad looks creepy
</message>
```

Good:

```xml
<message name="IDS_AD_REPORT_REASON" desc="Option in the ad report menu">
  This ad looks suspicious
</message>
```
