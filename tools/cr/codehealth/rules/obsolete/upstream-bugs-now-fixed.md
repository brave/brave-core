---
name: obsolete/upstream-bugs-now-fixed
rubric: obsolete
severity: obsolete
---

**Rule.** A Brave workaround, or test filter, that cites a Chromium bug or CL
the range fixed goes away with it.

**Why.** Workarounds outlive the bug they waited on, and a filter on a fixed
test hides coverage. Some workarounds cite their bug only in a Brave commit's
message (the Linux EULA plaster).

**How to check.** The candidates are
`vpython3 tools/cr/codehealth/scripts/obsolete.py cited-bugs`: each crbug and CL
Brave cites (in `chromium_src/`, `patches/` `+` lines, `rewrite/`,
`test/filters/`, comments, and the bodies of Brave's commits on its overrides)
that a range commit names in `Bug:`, `Fixed:` or `Reviewed-on:`, or that a
revert or reland quotes. `Fixed:`, and a `Reviewed-on:` of the CL Brave waited
on, are the strong signals; a `Bug:` only says the commit is related. Umbrella
bugs (Bedrock, exit-time destructors) are skipped. Check reverts before deleting
anything: the tool marks commits reverted later in the range.

## Examples

- The `layout_manager_base.cc` plaster waited on a CL whose reland made it a
  no-op.
