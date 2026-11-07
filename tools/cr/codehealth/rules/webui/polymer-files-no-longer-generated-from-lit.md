---
name: webui/polymer-files-no-longer-generated-from-lit
rubric: webui
severity: regression
---

**Rule.** When upstream replaces a shared Polymer file built from its Lit twin
with a standalone copy, Brave's override of the Lit file is applied to the copy
too.

**Why.** Some Polymer shared files are built from their Lit twin, so Brave's
override of the Lit file reaches both. A standalone copy, often made while
moving it ("Move shared Polymer files to last clients"), loses Brave's override.

**How to check.** Each candidate deletes or moves a WebUI file Brave overrides.
Match
`git diff -M --diff-filter=DR --name-status $A $B -- ui/webui/resources chrome/browser/resources`
against `chromium_src/`, and check where the file went.

## Examples

- M158, 8127.2: `md_select.css` moved to `settings_shared/`, and Brave's selects
  lost the Nala style.
