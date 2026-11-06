---
name: enforcement/v8-deprecations
rubric: enforcement
severity: deprecation
---

**Rule.** Brave doesn't call V8 APIs that the V8 range deprecates, or promotes
from `V8_DEPRECATE_SOON` to `V8_DEPRECATED`.

**Why.** `V8_DEPRECATED` breaks the build; `V8_DEPRECATE_SOON` is silent, as
`//.gn` sets `v8_imminent_deprecation_warnings = false`, so only this review
sees it before the promotion. Removed markers show promotions, and a removed
`V8_DEPRECATED` is usually the API's removal.

**How to check.** The candidates are the output of
`tools/cr/codehealth/scripts/v8_deprecations.py`, which reads `v8_revision` in
`src/DEPS` at both tags and runs, in `src/v8`,
`git log -p <v8 range> -- include/ | grep -E '^[+-].*V8_DEPRECATE|^commit '`,
with the declaration after each marker. For each deprecated declaration, grep
Brave for its callers (`git grep -nw '<Name>'`), and file an item for each one
still using it.
