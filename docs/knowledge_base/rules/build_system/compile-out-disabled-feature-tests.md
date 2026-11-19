---
name: brave/compile-out-disabled-feature-tests
description:
  'When a build flag disables a feature, its tests are excluded from the build
  under that flag, not kept compiling by adding deps for them.'
rubric: testing
severity: should_fix
---

**Rule.** List a feature's test files under the feature's build flag
(`if (enable_brave_rewards) { sources += [ ... ] }`), so they are not built when
the flag is off. Do not fix compilation errors of those tests in the disabled
configuration by adding deps they would otherwise not get.

**Why.** A feature that is compiled out has nothing to test. Adding deps just
for its tests pulls the disabled feature's code back into the build, which
defeats the build flag.

## Examples

Bad:

```gn
deps += [ "//brave/components/brave_rewards" ]  # just for tests
```

Good:

```gn
if (enable_brave_rewards) {
  sources += [ "rewards_service_unittest.cc" ]
}
```
