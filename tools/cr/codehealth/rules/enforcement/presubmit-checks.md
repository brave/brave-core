---
name: enforcement/presubmit-checks
rubric: enforcement
severity: deprecation
---

**Rule.** Brave's inlined `//PRESUBMIT.py` configuration, in
`chromium_presubmit_config.json5`, matches upstream's checks by name, and
Brave's call sites follow new bans.

**Why.** Brave inlines `//PRESUBMIT.py` and configures it by check name. A
renamed check leaves Brave's config entry matching nothing; a new
`treat_as_error` ban is an upcoming deprecation for each Brave call site; a
removed check makes the code written to dodge it, and the docs describing it,
obsolete.

**How to check.** A textual diff of `PRESUBMIT.py` is useless when upstream
reorders it. Instead, diff the set of `def Check*` names, each check's body and
the `_BANNED_*` lists between the two tags (`git show <tag>:PRESUBMIT.py`). For
a new ban, grep Brave for the banned name. For a renamed check, grep
`chromium_presubmit_config.json5` for the old name. For a changed check, replay
its logic over Brave files. For a removed check, grep Brave for its name.

## Examples

- M158, 8127.2: a `treat_as_error` ban on `DeprecatedLayoutImmediately()`.
- `CheckParseErrors` was renamed `CheckJSONParseErrors`.
- `CheckSettingsChanges` was removed, obsoleting Brave code and docs written
  around it.
