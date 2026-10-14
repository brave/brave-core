---
name: overlap/consolidated-entry-points
rubric: overlap
severity: regression
---

**Rule.** When upstream merges several entry points into a new one, the new one
still goes through Brave's `_ChromiumImpl` wrapper or override.

**Why.** The new entry point is called directly, so Brave's restriction on the
old ones silently stops applying.

**How to check.** For each embedder virtual Brave overrides that a candidate
touches, compare upstream's callers at both tags.

## Examples

- `AllowServiceWorker()` and friends became `IsAnyStorageAccessAllowed()`, which
  bypassed Request-OTR.
