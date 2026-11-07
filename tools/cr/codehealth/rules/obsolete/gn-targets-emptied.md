---
name: obsolete/gn-targets-emptied
rubric: obsolete
severity: obsolete
---

**Rule.** Brave's GN additions to an upstream target still do something after
upstream turns the target into an aggregator.

**Why.** An aggregator has no sources, `allow_circular_includes_from` or deps of
its own, so Brave's `deps +=`, `allow_circular_includes_from` and `sources`
patches on it stop doing anything, or move deps after the lock.

**How to check.** The candidates empty or lock down targets in `BUILD.gn` files
Brave patches or plasters. Read the target's upstream comment at the new tag for
each Brave `patches/**/BUILD.gn.patch` and `rewrite/**/BUILD.gn.yaml` on it.

## Examples

- M157, 8093.0: `//chrome/browser/ui:ui`.
- M158, 8109.1: a `sources` plaster on the `sources = []`
  `//chrome/test:test_support`.
