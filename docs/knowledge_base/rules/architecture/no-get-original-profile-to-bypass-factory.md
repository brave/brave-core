---
name: brave/no-get-original-profile-to-bypass-factory
description:
  'Never call GetOriginalProfile() or a similar method to obtain a service that
  its factory declines to give the current profile.'
rubric: security
severity: blocking
---

**Rule.** NEVER switch to the original (regular) profile with
`GetOriginalProfile()`, `GetOriginalBrowserContext()` or similar in order to get
a service its factory returns null for on the current profile. Ask the factory
with the current profile and handle a null result.

**Why.** A factory returns null for a profile type, such as incognito, because
that profile must not use the service; reaching it through the original profile
defeats the check, for example leaking incognito activity into the regular
profile's service.

## Examples

Bad:

```cpp
auto* profile = Profile::FromBrowserContext(context)->GetOriginalProfile();
auto* service = MyServiceFactory::GetForProfile(profile);
```

Good:

```cpp
auto* service =
    MyServiceFactory::GetForProfile(Profile::FromBrowserContext(context));
if (!service) {
  return;
}
```
