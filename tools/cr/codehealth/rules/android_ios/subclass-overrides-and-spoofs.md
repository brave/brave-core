---
name: android_ios/subclass-overrides-and-spoofs
rubric: android_ios
severity: regression
---

**Rule.** Brave Java overrides are still called, and a hook that pretends to be
something is still asked by upstream.

**Why.** Upstream moving a call to another object leaves Brave's override
compiling but dead, with no error anywhere.

**How to check.** For each candidate commit that moves or removes a method a
Brave subclass overrides (`git grep -n '<method>(' -- '*.java'` in Brave), trace
whether upstream still calls it on the object Brave overrides.

## Examples

- `TabImpl.isCustomTab()` moved to the tab's delegate factory, which killed
  Brave's reader mode prompt.
