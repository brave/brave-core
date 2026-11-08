---
name: brave/ui-text-brave-capitalized
description:
  '"Brave" is always capitalized in user-facing text, except in URLs such as
  "brave.com".'
rubric: readability
severity: should_fix
---

**Rule.** Always capitalize "Brave" (not "brave" or "BRAVE"), except when
writing the URL "brave.com", and so in other URLs such as `brave://settings`.

**Why.** "Brave" is a proper noun and the brand name.

## Examples

Bad:

```xml
<message name="IDS_SUPPORT_LINK" desc="Link to the support site">
  Get help from the brave community
</message>
```

Good:

```xml
<message name="IDS_SUPPORT_LINK" desc="Link to the support site">
  Get help from the Brave community
</message>
```
