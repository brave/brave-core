---
name: brave/ui-text-brave-browser-lowercase
description:
  The product is written "Brave browser", with "browser" always lowercase.
rubric: readability
severity: should_fix
---

**Rule.** Write "Brave browser" with a lowercase "browser", NEVER "Brave
Browser".

**Why.** It is the established brand name; mixing forms makes the product name
look inconsistent.

## Examples

Bad:

```xml
<message name="IDS_UPDATE_TITLE" desc="Title of the update notice">
  The Brave Browser is up to date
</message>
```

Good:

```xml
<message name="IDS_UPDATE_TITLE" desc="Title of the update notice">
  The Brave browser is up to date
</message>
```
