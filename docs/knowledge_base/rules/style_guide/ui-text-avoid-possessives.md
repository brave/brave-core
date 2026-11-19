---
name: brave/ui-text-avoid-possessives
description:
  User-facing strings avoid possessive constructions, preferring "the content of
  this page" to "this page's contents".
rubric: readability
severity: nit
---

**Rule.** Avoid possessive constructions on things: write "the content of this
page", not "this page's contents". "Brave's" in place of "our" is the accepted
exception.

**Why.** Possessives translate poorly: many languages express them differently,
and the result reads awkwardly.

## Examples

Bad:

```xml
<message name="IDS_SPEEDREADER_DESC" desc="Description of the Speedreader setting">
  Speedreader simplifies this page's contents.
</message>
```

Good:

```xml
<message name="IDS_SPEEDREADER_DESC" desc="Description of the Speedreader setting">
  Speedreader simplifies the content of this page.
</message>
```
