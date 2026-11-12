---
name: obsolete/dead-overrides
rubric: obsolete
severity: obsolete
---

**Rule.** Every `chromium_src` file, plaster, lit mangler and
`build/chromium/resources/` file still has an upstream target, and every entry
in `base/compile_overridden_features.inc` names an upstream feature.

**Why.** Nothing checks these: an override whose target was deleted or moved is
dead code, and a stale feature entry overrides nothing.

**How to check.** The candidates are
`vpython3 tools/cr/codehealth/scripts/obsolete.py removed-names`, rows
`override` (targets the range deleted or moved) and `feature`
(`removed in range`, or `missing at both`, listed with `--debt`). For a move,
port the override to the new path; otherwise delete it. Polymer overrides are
keyed by element tag: sweep [d](../../SKILL.md#d-webui) checks them.

## Examples

- M158: `ShowRelatedWebsiteSetsPermissionGrants` in
  `compile_overridden_features.inc`, removed upstream in the range.
