---
name: brave/handle-disabled-features-internally
description:
  'When a feature can be disabled, its factory or service returns null or does
  nothing, instead of every caller wrapping its uses in #if BUILDFLAG guards.'
rubric: design
severity: should_fix
---

**Rule.** When a feature can be disabled, make its factory or service handle the
disabled state, returning null or doing nothing, and have callers check the
result at runtime. Do not make each caller wrap its uses of the feature in
`#if BUILDFLAG(...)` / `#ifdef` guards.

**Why.** Build-flag guards scattered across callers lead to missing build
dependencies and are a maintenance burden; a single internal check keeps the
callers simple.

## Examples

Bad:

```cpp
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  rewards_service->DoSomething();
#endif
```

Good:

```cpp
auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
if (rewards_service) {  // Returns null when disabled
  rewards_service->DoSomething();
}
```
