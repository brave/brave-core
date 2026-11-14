---
name: brave/group-gn-sources-with-their-deps
description:
  'In BUILD.gn, sources and the deps they need belong in the same feature-sized
  target, not added to one large catch-all target.'
rubric: design
severity: should_fix
---

**Rule.** Keep the `sources` of a feature and the `deps` those sources need in
the same target block. When a change adds a feature's files to a large root
target (`source_set("browser")` with hundreds of files and deps), move them into
a feature target of their own, in a separate `BUILD.gn` if needed, and depend on
that target instead.

**Why.** In a catch-all target nobody can tell which deps belong to which
sources, so stale deps are never removed and a feature's dependencies cannot be
reviewed or guarded as a unit.

## Examples

Bad:

```gn
source_set("browser") {
  sources = [ ... 200 files ... ]
  deps = [ ... 100 deps ... ]
}
```

Good:

```gn
source_set("branded_wallpaper") {
  sources = [ "branded_wallpaper.cc" ]
  deps = [ "//brave/components/ntp_background_images" ]
}
```
