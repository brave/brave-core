---
name: brave/ui-text-no-ampersand-for-and
description:
  User-facing strings spell out "and" rather than using an ampersand, except in
  "Trackers & ads".
rubric: readability
severity: nit
---

**Rule.** Write "and", not "&". The one accepted exception is "Trackers & ads",
which highlights how closely the two are related.

**Why.** An ampersand is shorthand; spelled out, the text reads naturally and
translates without ambiguity.

## Examples

Bad:

```xml
<message name="IDS_SETTINGS_PRIVACY_TITLE" desc="Title of the privacy settings section">
  Privacy &amp; security
</message>
```

Good:

```xml
<message name="IDS_SETTINGS_PRIVACY_TITLE" desc="Title of the privacy settings section">
  Privacy and security
</message>
```
