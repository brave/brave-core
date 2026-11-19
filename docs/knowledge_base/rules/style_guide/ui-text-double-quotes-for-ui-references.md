---
name: brave/ui-text-double-quotes-for-ui-references
description:
  User-facing instructions put the UI copy they refer to in double quotes, such
  as Click "Search options".
rubric: readability
severity: nit
---

**Rule.** When an instruction refers to UI copy, such as a button or setting
name, put that copy in double quotes: Click "Search options". Do not use single
quotes or leave it unquoted.

**Why.** Quoting marks exactly which words are on screen, so the reader can find
them and translators keep them consistent with the UI.

## Examples

Bad:

```xml
<message name="IDS_SEARCH_HELP" desc="Help text in search settings">
  Click 'Search options' to change your default engine.
</message>
```

Good:

```xml
<message name="IDS_SEARCH_HELP" desc="Help text in search settings">
  Click "Search options" to change your default engine.
</message>
```
