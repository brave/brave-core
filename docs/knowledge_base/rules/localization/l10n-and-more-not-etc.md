---
name: brave/l10n-and-more-not-etc
description:
  User-facing strings end an open list with "and more" rather than "etc.".
rubric: readability
severity: nit
---

**Rule.** End an open-ended list with "and more", not "etc.": "bookmarks,
passwords, and more".

**Why.** "And more" reads naturally in UI text and translates cleanly; "etc." is
an abbreviation many readers skim past.

## Examples

Bad:

```xml
<message name="IDS_IMPORT_DATA_TYPES" desc="Description of the import dialog">
  Import bookmarks, passwords, etc.
</message>
```

Good:

```xml
<message name="IDS_IMPORT_DATA_TYPES" desc="Description of the import dialog">
  Import bookmarks, passwords, and more
</message>
```
