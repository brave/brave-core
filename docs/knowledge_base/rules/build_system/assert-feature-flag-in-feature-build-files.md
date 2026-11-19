---
name: brave/assert-feature-flag-in-feature-build-files
description:
  'A BUILD.gn file that belongs entirely to a feature starts with
  assert(enable_feature) instead of wrapping its blocks in if (enable_feature).'
rubric: readability
severity: should_fix
---

**Rule.** When everything in a `BUILD.gn` file belongs to one feature, put
`assert(enable_feature)` at the top of the file. Do not wrap its targets in
`if (enable_feature) { ... }` blocks.

**Why.** The assert states once that the file is only for builds with the
feature, and fails loudly if something depends on it without the flag; a
top-level conditional silently drops the targets, so a misplaced dependency
fails later with a confusing "unknown target" error.

## Examples

Bad:

```gn
if (enable_brave_wallet) {
  source_set("wallet_tests") {
    sources = [ ... ]
  }
}
```

Good:

```gn
assert(enable_brave_wallet)

source_set("wallet_tests") {
  sources = [ ... ]
}
```
