---
name: brave/build-gn-patch-for-circular-java-sources
description:
  Java sources are added through java_sources.gni lists; a direct BUILD.gn patch
  adds them only when a circular dependency rules the list out.
rubric: change_scope
severity: should_fix
---

**Rule.** Add Brave Java sources to upstream targets through the
`java_sources.gni` lists (such as `brave_java_sources.gni`). Use a direct patch
of an upstream `BUILD.gn` to add sources only when a circular dependency
prevents the `java_sources.gni` route, and say so in the change.

**Why.** The source lists are the mechanism Brave maintains for this, and need
no patch. A `BUILD.gn` patch is the fallback for the case they cannot serve:
sources whose target would otherwise form a dependency cycle.

## Examples

Bad:

```diff
# patches/chrome-android-BUILD.gn.patch, for a source with no cycle
   sources += chrome_java_sources
+  sources += [ "//brave/android/java/org/chromium/chrome/browser/BraveFoo.java" ]
```

Good:

```gn
# android/brave_java_sources.gni
brave_java_sources += [
  "../../brave/android/java/org/chromium/chrome/browser/BraveFoo.java",
]
```
