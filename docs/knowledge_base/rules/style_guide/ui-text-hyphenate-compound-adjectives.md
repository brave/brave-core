---
name: brave/ui-text-hyphenate-compound-adjectives
description:
  User-facing strings hyphenate multi-word adjectives before a noun (but not
  after it), and repeated-letter words such as "re-enable".
rubric: readability
severity: nit
---

**Rule.** Hyphenate a compound adjective before the noun it modifies
("up-to-date browser", "non-compatible extension"), and leave it unhyphenated
after the noun ("browser is up to date"). Hyphenate repeated-letter words
("re-enable"), but prefer wording that avoids them.

**Why.** Translators work from the English source, and a missing or misplaced
hyphen changes how a phrase parses.

## Examples

Bad:

```xml
<message name="IDS_EXTENSION_WARNING" desc="Warning for an incompatible extension">
  This is a non compatible extension. Make sure your browser is up-to-date.
</message>
```

Good:

```xml
<message name="IDS_EXTENSION_WARNING" desc="Warning for an incompatible extension">
  This is a non-compatible extension. Make sure your browser is up to date.
</message>
```
