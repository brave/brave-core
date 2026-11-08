---
name: brave/no-patches-to-python-build-scripts
description:
  'Python build scripts are not patched; the change is made with a chromium_src
  override, a header patch or a #define-based approach instead.'
rubric: correctness
severity: blocking
---

**Rule.** NEVER patch a Python build script or build tool (such as
`build/android/gyp/java_cpp_enum.py`). Prefer, in order, a `chromium_src/`
override, a multiline patch to the header the script reads, or a `#define`-based
approach. For Java or C++ enums that an upstream Python script processes, a
multiline patch to the enum's header is acceptable.

**Why.** A patched build script prevents correct incremental rebuilds and breaks
remote `siso` build actions.

## Examples

Bad:

```diff
// patches/build-android-gyp-java_cpp_enum.py.patch
+    if enum_name == 'FooType': entries.extend(BRAVE_FOO_TYPES)
```

Good:

```diff
// patches/components-foo-foo_type.h.patch
 enum class FooType {
   kBar,
+  kBraveBaz,
+  kBraveQux,
-  kMaxValue = kBar,
+  kMaxValue = kBraveQux,
```
