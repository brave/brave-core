---
name: brave/ui-text-punctuation-inside-quotes
description:
  In user-facing text, commas, exclamation points and periods go inside
  quotation marks.
rubric: readability
severity: nit
---

**Rule.** Place commas, exclamation points and periods inside the closing
quotation mark: Click "Save." not Click "Save".

**Why.** It is the convention of US English, which Brave's source strings
follow.

## Examples

Bad:

```xml
<message name="IDS_BACKUP_HELP" desc="Help text in the backup dialog">
  To finish, click "Back up".
</message>
```

Good:

```xml
<message name="IDS_BACKUP_HELP" desc="Help text in the backup dialog">
  To finish, click "Back up."
</message>
```
