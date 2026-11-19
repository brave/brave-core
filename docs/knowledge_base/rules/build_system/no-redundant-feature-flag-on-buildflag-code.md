---
name: brave/no-redundant-feature-flag-on-buildflag-code
description:
  "Code already compiled only under a feature's build flag does not get a
  redundant runtime base::Feature check on top."
rubric: design
severity: should_fix
---

**Rule.** Do not add a `base::Feature` and a `base::FeatureList::IsEnabled()`
check whose only purpose is to turn off code that is already compiled only when
the feature's build flag is set (inside `#if BUILDFLAG(ENABLE_FOO)`, or in files
built only under `if (enable_foo)`). The build flag already decides whether that
code exists.

**Why.** Build flags and runtime feature flags are different mechanisms. A
second, runtime switch for the same decision adds a state in which the feature
is built but off, which nobody tests and which has to be removed later.

## Examples

Bad:

```cpp
#if BUILDFLAG(ENABLE_BRAVE_FOO)
  if (base::FeatureList::IsEnabled(features::kBraveFoo)) {
    FooService::Start();
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_FOO)
```

Good:

```cpp
#if BUILDFLAG(ENABLE_BRAVE_FOO)
  FooService::Start();
#endif  // BUILDFLAG(ENABLE_BRAVE_FOO)
```
