---
name: brave/one-gn-guard-per-target
description:
  'A BUILD.gn target opens at most one block per condition; additions under a
  condition the target already tests go into its existing block.'
rubric: readability
severity: should_fix
---

**Rule.** Within one GN target, do not repeat a guard: when the target already
has an `if (enable_brave_foo)` block, add the new `sources`, `deps` or other
entries to that block instead of opening a second `if (enable_brave_foo)`.

**Why.** Repeated blocks for one condition scatter a feature's build
configuration across the target, so readers miss part of it and later changes
update one copy but not the other.

## Examples

Bad:

```gn
source_set("browser") {
  if (enable_brave_foo) {
    sources += [ "foo_service.cc" ]
  }
  deps = [ "//base" ]
  if (enable_brave_foo) {
    deps += [ "//brave/components/foo" ]
  }
}
```

Good:

```gn
source_set("browser") {
  deps = [ "//base" ]
  if (enable_brave_foo) {
    sources += [ "foo_service.cc" ]
    deps += [ "//brave/components/foo" ]
  }
}
```
