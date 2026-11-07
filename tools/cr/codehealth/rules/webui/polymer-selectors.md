---
name: webui/polymer-selectors
rubric: webui
severity: regression
---

**Rule.** For tags still on Polymer, each `querySelector` target in Brave's
template modification still exists in upstream's template at the new tag, at the
level Brave queries it.

**Why.** A query on the template fragment doesn't reach into nested `<template>`
(`dom-if`) content, and attribute selectors behind `?.` fail silently: the
modification never happens, and nothing reports it.

**How to check.** Take the `fine` rows of `polymer_tags.py` (shared with
`polymer-overrides-on-lit-elements`). The candidates are the range's commits to
those elements' templates. For each, compare the selectors in Brave's
`Register*` callback with the upstream template at the new tag
(`git show <tag>:<path>.html`), including whether the target moved into a
`dom-if`.

## Examples

- M157, 8097.1: `br/site_details.ts` matched no `settings:vr-headset` icon since
  M129.
