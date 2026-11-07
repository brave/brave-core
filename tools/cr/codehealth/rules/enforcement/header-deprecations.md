---
name: enforcement/header-deprecations
rubric: enforcement
severity: deprecation
---

**Rule.** Brave doesn't call upstream APIs that the range marks as deprecated in
their headers, or that new header guidance tells callers to avoid.

**Why.** Deprecations announce the next lift's build breaks. Comment-only
deprecations never break the build; `[[deprecated]]`, `V8_DEPRECATE*` and
abseil's `diagnose_if` do. Guidance that never says "deprecated" (`Prefer`,
`Avoid`, `New callers`, `TODO(...): Migrate callers`) is the same signal,
earlier.

**How to check.** The candidates are header lines the range adds, matching
`[Dd]eprecat|DEPRECAT|[Dd]o not use|being removed|will be removed|\[\[deprecated`
or `TODO\([^)]*\): [Mm]igrate (callers|users)`, plus a second pass with
`[Pp]refer |[Aa]void |[Nn]ew callers` over `base/`, `content/public/`,
`ui/views/`, `net/` and `mojo/public/`. By hand:
`git log -p -E -G'<pattern>' $A..$B -- '*.h'`, keeping only added lines (git's
ERE has no `(?i)`). For each, read the declaration the comment belongs to
(`git show <hash> -- <header>`), and grep Brave for its callers.

## Examples

- `RenderFrameHost::ConsumeTransientUserActivation()`, found by the guidance
  pass.
- M159, 8144.0: `TabModelList::models()`, kept "temporarily" behind a
  `TODO(crbug.com/580974657): Migrate callers`.
