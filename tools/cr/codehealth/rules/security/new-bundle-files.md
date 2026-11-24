---
name: security/new-bundle-files
rubric: security
severity: regression
---

**Rule.** New files in the Mac app bundle survive Brave's universal build, and
Brave's signing still matches the bundle's code-signed parts.

**Why.** Brave's Mac universal build copies the other architecture's app with
ignore patterns (`installer/mac/copy_other_arch_binary.py`). A new
per-architecture file that matches a pattern ships for one architecture only.
Brave's `script/signing_helper.py` overrides parts of
`chrome/installer/mac/signing/parts.py` by name; no build or test runs it.

**How to check.** Check the files the candidate commits add to
`chrome/chrome_paks.gni` and other bundle data against the ignore patterns in
`installer/mac/copy_other_arch_binary.py`. For changes to `parts.py`, check that
every part `BraveModifyPartsForSigning()` names still exists, and that the
options Brave signs a part with still fit what it now loads.

## Examples

- M158: per-architecture `webui_code_cache.*.pak` matched `*.pak`.
- M160, 8169.0: the `helper-aperitif-*` parts removed, which Brave's signing
  still writes, and every helper now links `libaperitif.dylib`.
