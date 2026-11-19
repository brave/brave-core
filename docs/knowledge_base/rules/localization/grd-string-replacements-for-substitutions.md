---
name: brave/grd-string-replacements-for-substitutions
description:
  A simple word or phrase substitution across upstream strings is made with
  grd_string_replacements.py and chromium_rebase_l10n, not with GRD edits or
  chromium_src overrides.
rubric: design
severity: should_fix
---

**Rule.** When upstream strings only need a word or phrase replaced (such as a
product name), add the substitution to `default_replacements` in
`brave/script/lib/l10n/grd_string_replacements.py` and run
`pnpm run chromium_rebase_l10n`. Do not add Brave copies of those strings with
`chromium_src` `IDS_` overrides, or edit GRD files by hand, for it.

**Why.** The replacement applies consistently to every matching string in all
locales, and keeps applying after rebases; per-string overrides miss new strings
and duplicate translations.

## Examples

Bad:

```cpp
// chromium_src/chrome/browser/ui/foo/foo_view.cc
#undef IDS_FOO_SYNC_TITLE
#define IDS_FOO_SYNC_TITLE IDS_BRAVE_FOO_SYNC_TITLE  // "Chrome" -> "Brave"
```

Good:

```python
# brave/script/lib/l10n/grd_string_replacements.py
default_replacements = [
    ...
    (r'Google Chrome', 'Brave'),
]
```
