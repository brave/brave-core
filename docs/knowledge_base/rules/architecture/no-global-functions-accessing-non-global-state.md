---
name: brave/no-global-functions-accessing-non-global-state
description:
  'Free functions and static methods do not reach into global registries for
  profile- or window-specific data; they take that data as a parameter.'
rubric: design
severity: should_fix
---

**Rule.** A free function or static method that needs profile- or
window-specific data takes it, or the service holding it, as a parameter. It
does not look it up through global registries such as
`ProfileManager::GetLastUsedProfile()`, `BrowserList` or
`chrome::FindLastActive()`.

**Why.** A global lookup picks whichever profile or window happens to be last
used or active, which is often the wrong one, and hides the function's real
input from its callers and tests.

## Examples

Bad:

```cpp
bool IsFeatureReady() {
  auto* profile = ProfileManager::GetLastUsedProfile();
  return MyServiceFactory::GetForProfile(profile)->IsReady();
}
```

Good:

```cpp
bool IsFeatureReady(MyService* service) {
  return service->IsReady();
}
```
