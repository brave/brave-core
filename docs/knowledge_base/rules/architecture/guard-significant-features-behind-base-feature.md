---
name: brave/guard-significant-features-behind-base-feature
description:
  'Significant, experimental or risky new functionality is guarded by a
  base::Feature so it can be disabled remotely; small self-contained changes and
  simple fixes need none.'
rubric: design
severity: should_fix
---

**Rule.** When a change adds a large or complex feature with broad impact, or
experimental or risky functionality that may need rolling back, guard it with a
`base::Feature` checked at its entry points, so that Griffin can disable it
remotely. Use judgment: do NOT ask for a flag on small, self-contained changes
or simple bug fixes.

**Why.** Without a runtime flag, a crash or regression in new functionality can
only be fixed by shipping a new build; a feature flag is a safety net that turns
it off in the field.

## Examples

Bad:

```cpp
auto* service = MyNewServiceFactory::GetForProfile(profile);
service->DoSomething();  // Crashes if service unavailable
```

Good:

```cpp
if (!base::FeatureList::IsEnabled(features::kMyNewFeature)) {
  return;
}
auto* service = MyNewServiceFactory::GetForProfile(profile);
if (!service) {
  return;
}
service->DoSomething();
```
