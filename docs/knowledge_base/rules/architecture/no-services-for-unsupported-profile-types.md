---
name: brave/no-services-for-unsupported-profile-types
description:
  'A keyed service factory returns null for profile types its service must not
  run for, such as incognito, through ProfileSelections or
  GetBrowserContextToUse rather than a check added to GetForProfile.'
rubric: correctness
severity: blocking
---

**Rule.** A factory MUST NOT create its service for profile types the service
does not support (for example Rewards for incognito). Express this in the
factory's profile selection: the `ProfileSelections` of a
`ProfileKeyedServiceFactory` (such as `ProfileSelection::kOriginalOnly`), or a
`GetBrowserContextToUse()` that returns null for those profiles. Do not instead
add the profile-type check to the static `GetForProfile()` accessor. Flag a
`GetBrowserContextToUse()` that returns the context unconditionally.

**Why.** A service created for an unsupported profile, such as incognito, runs
where it must not, which is a correctness and privacy problem. The profile
selection is the one place that decides it, for every way of reaching the
service.

## Examples

Bad:

```cpp
content::BrowserContext* GetBrowserContextToUse(
    content::BrowserContext* context) const override {
  return context;  // This creates services for incognito!
}
```

Good:

```cpp
content::BrowserContext* GetBrowserContextToUse(
    content::BrowserContext* context) const override {
  if (context->IsOffTheRecord()) {
    return nullptr;
  }
  return context;
}
```
