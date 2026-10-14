---
name: migrations/scripted-campaigns-and-whole-file-replacements
rubric: migrations
severity: regression
---

**Rule.** Every `chromium_src` file that replaces an upstream file outright gets
the conversion a scripted campaign applied to the original.

**Why.** The script only rewrites upstream's text. Brave's replacement keeps the
old idiom, and breaks once the campaign removes it, often only on the platform
the file is for.

**How to check.** In the candidate list, `Brave overrides:` paths marked `*` are
whole-file replacements a campaign touched. Diff what the campaign did to the
original (`git show <hash> -- <path>`) and check that Brave's copy has the same
change.

## Examples

- M158, 8109.1: the typed-factory conversion broke Brave's
  `GoogleGroupsManagerFactory` stub on iOS.
