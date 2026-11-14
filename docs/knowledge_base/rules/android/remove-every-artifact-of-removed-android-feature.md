---
name: brave/remove-every-artifact-of-removed-android-feature
description:
  A change removing an Android feature removes all its artifacts, from bytecode
  adapters, ProGuard rules and bytecode tests to Java sources, resources, JNI
  bindings, feature flags and build references.
rubric: change_scope
severity: should_fix
---

**Rule.** A change that removes a feature from Android MUST also remove every
artifact the feature leaves: its bytecode class adapters, ProGuard rules,
`BytecodeTest` entries, Java source files, resource files, JNI bindings, feature
flags and build system references (`brave_java_sources.gni`, `BUILD.gn`,
resource lists). Check each of these integration points.

**Why.** Artifacts left behind keep dead code built and shipped, keep ProGuard
rules alive for classes that no longer exist, and leave adapters and tests
naming classes that are gone.

## Examples

Bad:

```gn
# BraveFooActivity.java is deleted, but its source stays listed.
brave_java_sources += [
  "../../brave/android/java/org/chromium/chrome/browser/foo/BraveFooActivity.java",
]
```

Good:

```
Deleted together: BraveFooActivity.java, BraveFooClassAdapter.java, its
BytecodeTest entries, its -keep rule in proguard.flags, its layouts, its JNI
bridge and kBraveFoo.
```
