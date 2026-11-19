---
name: brave/ui-text-no-click-here
description:
  Link text must never be "click here"; it describes what happens when the link
  is followed.
rubric: readability
severity: should_fix
---

**Rule.** NEVER use "click here" (or "tap here", or a bare "here") as link text.
Make the linked words describe what happens when the link is followed, such as
the action or the page it opens.

**Why.** Screen reader users often navigate by a list of links, where "click
here" says nothing. Descriptive link text is also clearer to everyone else.

## Examples

Bad:

```xml
<message name="IDS_EMAIL_UNSUBSCRIBE" desc="Footer of marketing emails">
  To unsubscribe from these emails, <ph name="BEGIN_LINK">$1</ph>click here<ph name="END_LINK">$2</ph>.
</message>
```

Good:

```xml
<message name="IDS_EMAIL_UNSUBSCRIBE" desc="Footer of marketing emails">
  Want to stop getting these emails? <ph name="BEGIN_LINK">$1</ph>Unsubscribe<ph name="END_LINK">$2</ph>.
</message>
```
