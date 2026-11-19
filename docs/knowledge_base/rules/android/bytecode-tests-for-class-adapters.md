---
name: brave/bytecode-tests-for-class-adapters
description:
  A change adding or modifying a bytecode class adapter adds BytecodeTest checks
  that the classes it names exist and that the methods it touches exist with the
  right return types.
rubric: testing
severity: blocking
---

**Rule.** A change that adds or modifies a class adapter in
`build/android/bytecode/java/org/brave/bytecode/` MUST add the matching checks
to `android/javatests/org/chromium/chrome/browser/BytecodeTest.java`: a
`classExists()` assertion in `testClassesExist` for each class the adapter
names, and a `methodExists()` assertion with the correct return type in
`testMethodsExist` for each method it changes, redirects or moves.

**Why.** An adapter names upstream classes and methods by string. When an
upstream refactor renames or changes one, the adapter silently stops applying
and the build still succeeds; the bytecode test is what turns that into a test
failure instead of a runtime failure in the shipped browser.

## Examples

Bad:

```java
// BraveAdaptiveToolbarPrefsClassAdapter.java is added; BytecodeTest is unchanged.
changeMethodOwner(
        sAdaptiveToolbarPrefsClassName,
        "getCustomizationSetting",
        sBraveAdaptiveToolbarPrefsClassName);
```

Good:

```java
// BytecodeTest.testClassesExist()
Assert.assertTrue(
        classExists("org/chromium/chrome/browser/toolbar/adaptive/AdaptiveToolbarPrefs"));
// BytecodeTest.testMethodsExist()
Assert.assertTrue(
        methodExists(
                "org/chromium/chrome/browser/toolbar/adaptive/AdaptiveToolbarPrefs",
                "getCustomizationSetting",
                MethodModifier.STATIC,
                int.class));
```
