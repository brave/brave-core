---
name: brave/sources-gni-only-for-upstream-cycles
description:
  'sources.gni is used only to insert files into an upstream Chromium target
  they have circular deps with; everything else goes in normal BUILD.gn targets.'
rubric: perf
severity: should_fix
---

**Rule.** Add files to a `sources.gni` only when they must be compiled into an
upstream Chromium target because they and that target depend on each other. In
every other case, put them in a normal `BUILD.gn` target and depend on it.

**Why.** Files listed in `sources.gni` become part of a large upstream target,
so changing one of them rebuilds that target, which hurts incremental builds;
they also escape the dependency checking of a target of their own.

## Examples

Bad:

```gn
# brave/components/foo/sources.gni: foo has no cycle with //chrome/browser.
brave_components_foo_sources = [ "//brave/components/foo/foo_service.cc" ]
```

Good:

```gn
# brave/components/foo/BUILD.gn
static_library("foo") {
  sources = [ "foo_service.cc" ]
  deps = [ "//base" ]
}
```
