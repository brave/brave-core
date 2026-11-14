---
name: brave/no-unrelated-deps-in-guards
description:
  "A GN dep goes inside a guard only when it is specific to that guard's
  condition; deps needed regardless of it are unconditional."
rubric: correctness
severity: should_fix
---

**Rule.** Put a dep inside an `if (...)` block only when the dep itself is
specific to that condition (for example `//content/public/browser` under
`if (use_blink)`). A dep the target needs whatever the condition is, such as a
general utility target, goes in the unconditional `deps`.

**Why.** A dep placed under an unrelated guard is missing on platforms where the
guard is false, although code there still uses it: the build has incorrect
configurations and fails with missing symbol errors on those platforms.

## Examples

Bad:

```gn
if (use_blink) {
  deps += [
    "//brave/components/constants",
    "//content/public/browser",
  ]
}
```

Good:

```gn
deps += [ "//brave/components/constants" ]
if (use_blink) {
  deps += [ "//content/public/browser" ]
}
```
