---
name: brave/grd-desc-gives-translators-context
description:
  A string's description gives translators, who see only the description, enough
  context to disambiguate it, without insisting on verbose descriptions for
  self-evident strings.
rubric: readability
severity: should_fix
---

**Rule.** Write each `desc` (or iOS `comment:`) so that a translator who sees
only the description can translate the string correctly. Qualify terms with
several meanings: "History" (browser or conversation?), "Wallet" (crypto or
payment?), "Send" (where?). For self-evident strings such as "Summary",
"Settings" or "Cancel", a brief description is fine: NEVER ask for a verbose
description when the string speaks for itself.

**Why.** Translators often see a string and its description without the UI. An
ambiguous word is translated with the wrong meaning, and the error ships in
every locale.

## Examples

Bad:

```xml
<message name="IDS_URL_PICKER_HISTORY" desc="History">
  History
</message>
```

Good:

```xml
<message name="IDS_URL_PICKER_HISTORY" desc="Title for the browser visits history section of a URL picker">
  History
</message>
```
