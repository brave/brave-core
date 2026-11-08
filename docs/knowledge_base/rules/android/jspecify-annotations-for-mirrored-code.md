---
name: brave/jspecify-annotations-for-mirrored-code
description:
  Java code mirrored and built in other environments uses org.jspecify
  annotations, not Chromium's org.chromium.build.annotations copies, and its
  target depends on JSpecify with chromium_code = false and enable_errorprone =
  true.
rubric: correctness
severity: should_fix
---

**Rule.** Java code that is mirrored and built in other environments MUST import
nullness annotations from `org.jspecify.annotations`, not Chromium's copies
under `org.chromium.build.annotations`. Its build target adds
`"//third_party/android_deps:org_jspecify_jspecify_java"` to `deps`, sets
`chromium_code = false` to drop the automatic dependency on `build_java`, and
sets `enable_errorprone = true` so that `chromium_code = false` does not also
disable Error Prone.

**Why.** Chromium's annotation copies exist only in Chromium's tree, so mirrored
code using them does not build elsewhere. Without `enable_errorprone = true`,
`chromium_code = false` would turn off the NullAway checking the annotations are
for.

## Examples

Bad:

```java
// Mirrored to a standalone library.
import org.chromium.build.annotations.Nullable;
```

Good:

```gn
deps += [ "//third_party/android_deps:org_jspecify_jspecify_java" ]

# Prevent automatic dep on build_java.
chromium_code = false

# Do not let chromium_code = false disable Error Prone.
enable_errorprone = true
```
