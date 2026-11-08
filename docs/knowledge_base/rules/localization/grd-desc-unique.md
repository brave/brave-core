---
name: brave/grd-desc-unique
description:
  Each string's description is specific to it; the same desc is not copy-pasted
  across strings with different purposes.
rubric: readability
severity: should_fix
---

**Rule.** Write a description for each string. Do not copy-paste the same `desc`
onto strings that serve different purposes, such as a dialog's title, body and
buttons.

**Why.** A copied description describes only one of the strings, and misleads
translators about the others.

## Examples

Bad:

```xml
<message name="IDS_CLEAR_DATA_TITLE" desc="Clear data dialog">
  Clear browsing data?
</message>
<message name="IDS_CLEAR_DATA_CONFIRM" desc="Clear data dialog">
  Clear data
</message>
```

Good:

```xml
<message name="IDS_CLEAR_DATA_TITLE" desc="Title of the clear browsing data dialog">
  Clear browsing data?
</message>
<message name="IDS_CLEAR_DATA_CONFIRM" desc="Confirm button of the clear browsing data dialog">
  Clear data
</message>
```
