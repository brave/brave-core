---
name: brave/gn-deps-via-brave-variable
description:
  A GN patch adding dependencies to an upstream target references a list
  variable defined in Brave code instead of listing the dependencies inline.
rubric: design
severity: should_fix
---

**Rule.** A hand-written patch that adds dependencies to an upstream GN target
adds one line, `deps += brave_<target>_deps`, referencing a list that a Brave
`.gni` defines. NEVER list `//brave/...` (or other) targets inline in the patch.

**Why.** Brave then adds and removes the target's dependencies in its own file
without touching the patch, which stays one stable line.

## Examples

Bad:

```gn
+  deps += [ "//brave/browser/ui/views/location_bar" ]
```

Good:

```gn
+  deps += brave_browser_window_deps

# In brave code:
brave_browser_window_deps = [
  "//brave/browser/ui/views/location_bar",
]
```
