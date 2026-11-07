---
name: overlap/python-reach
rubric: overlap
severity: regression
---

**Rule.** What a `chromium_src` Python override needs from upstream is still
there at the new tag: the functions and methods it overrides, with the same
parameters, the upstream globals it reads, and the text it matches.

**Why.** A Python override runs inside the upstream module that inlines it. A
renamed target or global raises on import, a changed signature breaks the call
through `original_function`, and a literal upstream no longer holds makes an
`assert` fail or a `.replace()` do nothing. None of it shows until something
imports the module, which the build may never do.

**How to check.** The candidate list is `python_reach.py`'s output. It only
reads what the range changes. For a `name`, find upstream's new name and
retarget the override, or drop it if upstream removed what it changed. For
`params`, check what the override passes to `original_function`. For `text`,
find what replaced the text: when the override's own edit is no longer needed,
as in the example, drop it rather than matching the new text. A literal listed
with `(in ...)` is one the override matches in another file. A data entry that
only feeds a removed upstream use is dead code to delete.

## Examples

- M159, 8142.0: `compile_db.py` asserted on `rewrapper(\.exe)` in
  `_CLANG_WRAPPER_CMD_LINE_RE`, which
  [6719bf879779](https://chromium.googlesource.com/chromium/src/+/6719bf87977959222c38b6900874d8742097b2ca)
  removed. Only `run_iwyu_test.py` imports the module, so the build passed.
- M159: `policy_json.py`'s `'frame_name'` config entry, whose `$3` replacement
  upstream removed.
