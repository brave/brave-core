---
name: brave/grd-desc-not-empty
description: Every GRD/GRDP <message> has a non-empty desc attribute.
rubric: readability
severity: should_fix
---

**Rule.** Every `<message>` element MUST have a `desc` attribute with a
non-empty description (and every iOS `NSLocalizedString` a non-empty
`comment:`). NEVER leave it empty or omit it.

**Why.** The description is the translator's only context; without one, the
string is translated blind.

## Examples

Bad:

```xml
<message name="IDS_NEWS_FOLLOW" desc="">
  Follow
</message>
```

Good:

```xml
<message name="IDS_NEWS_FOLLOW" desc="Button label that follows a Brave News channel">
  Follow
</message>
```
