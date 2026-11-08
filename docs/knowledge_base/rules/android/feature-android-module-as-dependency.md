---
name: brave/feature-android-module-as-dependency
description:
  A feature's Android module is its own android_library that consumers depend
  on, through brave_chrome_java_deps for chrome_java callers, never also listed
  in brave_java_sources and never depending back on chrome_java.
rubric: design
severity: should_fix
---

**Rule.** A feature-local Android module defines its own target (such as
`android_library("java")` in `browser/<feature>/android/BUILD.gn`), with
dependencies limited to the APIs it uses. Consumers depend on that target
instead of compiling its sources through `brave_java_sources`:

- for a caller compiled in `//chrome/android:chrome_java`, append the target to
  `brave_chrome_java_deps` in `build/android/config.gni`;
- for a caller in another module, add the target to that caller's `deps`.

The module MUST NOT depend back on `//chrome/android:chrome_java`, and its Java
files MUST NOT also be listed in `brave_java_sources`.

**Why.** A target with its own narrow dependencies is what makes the module
isolated; a reverse dependency on `chrome_java` would tie it back into the
monolith (and cycle with the hook that adds it there), and listing its files in
`brave_java_sources` as well would compile them into two targets.
`browser/brave_origin/android/BUILD.gn` is an existing example.

## Examples

Bad:

```gn
# android/brave_java_sources.gni
brave_java_sources += [
  "../../brave/browser/my_feature/android/java/src/org/chromium/chrome/browser/my_feature/MyFeatureController.java",
]
```

Good:

```gn
# browser/my_feature/android/BUILD.gn
android_library("java") {
  sources = [ "java/src/org/chromium/chrome/browser/my_feature/MyFeatureController.java" ]
  deps = [ "//base:base_java" ]
}

# build/android/config.gni
brave_chrome_java_deps += [ "//brave/browser/my_feature/android:java" ]
```
