---
name: brave/source-set-only-for-internal-targets
description:
  "A component's public targets are static_library or component; source_set is
  only for internal targets, which must restrict their visibility."
rubric: design
severity: should_fix
---

**Rule.** A target other code outside the component depends on is a
`static_library` or a `component`, not a `source_set`. A `source_set` is only
for the component's internal deps, and it MUST restrict its `visibility` (for
example `visibility = [ ":*" ]`) so nothing outside the component can depend on
it.

**Why.** Restricted visibility is what keeps an internal `source_set` internal:
without it, any target can depend on the component's implementation details, and
the component's real public API is no longer the targets it means to expose.

## Examples

Bad:

```gn
source_set("internal_network") {
  sources = [ ... ]
}
```

Good:

```gn
source_set("internal_network") {
  visibility = [ ":*" ]  # Only usable within this BUILD.gn
  sources = [ ... ]
}
```
