---
name: brave/separate-targets-for-accumulated-java-sources
description:
  When Java sources for one feature accumulate in brave_java_sources.gni, moving
  them into a separate build target is considered.
rubric: design
severity: nit
---

**Rule.** When a change adds to `brave_java_sources.gni` yet more Java sources
of a feature that already has many there (for example `crypto_wallet`), suggest
creating a separate build target for that feature's sources instead.

**Why.** A feature with its own target has its own dependencies, which isolates
its build and makes what it depends on explicit, instead of compiling it as part
of one large target.

## Examples

Bad:

```gn
brave_java_sources += [
  "../../brave/android/java/org/chromium/chrome/browser/crypto_wallet/FooActivity.java",
  "../../brave/android/java/org/chromium/chrome/browser/crypto_wallet/FooAdapter.java",
]
```

Good:

```gn
# The feature's own BUILD.gn, depended on instead of listing its sources.
android_library("java") {
  sources = [
    "java/src/org/chromium/chrome/browser/crypto_wallet/FooActivity.java",
    "java/src/org/chromium/chrome/browser/crypto_wallet/FooAdapter.java",
  ]
  deps = [ "//base:base_java" ]
}
```
