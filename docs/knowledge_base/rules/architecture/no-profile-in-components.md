---
name: brave/no-profile-in-components
description:
  'Code in components/ must not use Profile; it takes a PrefService (or the
  specific dependency), or a content::BrowserContext where a context is needed.'
rubric: design
severity: blocking
---

**Rule.** NEVER use `Profile` (`chrome/browser/profiles/profile.h`) in code
under `components/`. Take the `PrefService*` the code reads, obtained by the
caller with `user_prefs::UserPrefs::Get(browser_context)`, or the specific
service it uses; where the code genuinely needs a context, take a
`content::BrowserContext*`.

**Why.** `Profile` is the `//chrome` embedder's type, so a component using it
depends on the browser layer, which Chromium's layering forbids.

## Examples

Bad:

```cpp
// components/foo/browser/foo_service.cc
FooService::FooService(Profile* profile)
    : enabled_(profile->GetPrefs()->GetBoolean(kFooEnabled)) {}
```

Good:

```cpp
// components/foo/browser/foo_service.cc
FooService::FooService(PrefService* prefs) : prefs_(prefs) {}
```
