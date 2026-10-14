---
name: security/new-bundle-files
rubric: security
severity: regression
---

**Rule.** New files in the Mac app bundle survive Brave's universal build.

**Why.** Brave's Mac universal build copies the other architecture's app with
ignore patterns (`installer/mac/copy_other_arch_binary.py`). A new
per-architecture file that matches a pattern ships for one architecture only.

**How to check.** Check the files the candidate commits add to
`chrome/chrome_paks.gni` and other bundle data against the ignore patterns in
`installer/mac/copy_other_arch_binary.py`.

## Examples

- M158: per-architecture `webui_code_cache.*.pak` matched `*.pak`.
