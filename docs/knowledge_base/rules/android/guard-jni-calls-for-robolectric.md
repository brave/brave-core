---
name: brave/guard-jni-calls-for-robolectric
description:
  Java code that Robolectric tests exercise guards its JNI calls with a check
  such as FeatureList.isNativeInitialized(), since Robolectric has no native
  library.
rubric: testing
severity: should_fix
---

**Rule.** When a code path that a Robolectric test runs reaches a JNI call
(`ChromeFeatureList.isEnabled()`, a `*Jni.get()` native, a bridge), guard the
call with a conditional check such as `FeatureList.isNativeInitialized()` so
that the path handles the native-less test environment gracefully.
`BraveDynamicColors.java` is an existing example.

**Why.** Robolectric tests run on the JVM without the native library, so an
unguarded JNI call fails the test with an `UnsatisfiedLinkError` or a missing
native initialization instead of testing the Java logic.

## Examples

Bad:

```java
if (ChromeFeatureList.isEnabled(BraveFeatureList.SOME_FEATURE)) {
    applyFeature();
}
```

Good:

```java
if (FeatureList.isNativeInitialized()
        && ChromeFeatureList.isEnabled(BraveFeatureList.SOME_FEATURE)) {
    applyFeature();
}
```
