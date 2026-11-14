---
name: brave/grd-rebrand-upstream-product-names
description:
  In a Chromium rebase, new upstream strings that reference Google products are
  replaced with their Brave equivalents.
rubric: correctness
severity: should_fix
---

**Rule.** When a rebase brings new or changed upstream strings into Brave's
copies of upstream GRD/GRDP files, check them for Google product names (Gemini,
Google Pay, Chrome and so on) that users would see, and make sure Brave shows
its own equivalent, through `grd_string_replacements.py` or a Brave string with
a `chromium_src` override.

**Why.** Upstream strings ship to Brave users as they are, unless replaced: a
missed one shows Google's branding in Brave's UI.

## Examples

Bad:

```xml
<!-- app/generated_resources.grd, after a rebase -->
<message name="IDS_SETTINGS_AI_TITLE" desc="Title of the AI settings page">
  Gemini in Chrome
</message>
```

Good:

```xml
<!-- app/generated_resources.grd, after a rebase -->
<message name="IDS_SETTINGS_AI_TITLE" desc="Title of the AI settings page">
  Leo in Brave
</message>
```
