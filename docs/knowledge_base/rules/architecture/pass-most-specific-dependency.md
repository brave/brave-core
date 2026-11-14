---
name: brave/pass-most-specific-dependency
description:
  'Functions and classes take the most specific object they use (a PrefService*,
  the service itself), not a broader lookup key such as Profile*,
  BrowserContext* or WebContents* they extract it from.'
rubric: design
severity: should_fix
---

**Rule.** Pass a function or class the most specific object it uses, not a broad
lookup key it reaches that object through. Common substitutions: `Profile*` →
`PrefService*` when only prefs are read; `BrowserContext*` →
`scoped_refptr<SharedURLLoaderFactory>` when only the network is used;
`Profile*` → the service it fetched (`ExtensionSystem*`) when the profile is
held only to call `SomeService::Get(profile)`. The same applies to `WebContents`
and `TabInterface` used as lookup keys. Inject the key itself only when the code
genuinely needs it.

**Why.** A lookup key lets the code reach anything reachable from it, which
hides its real dependencies, couples it to unrelated services, and makes it hard
to test with fakes. A narrow parameter states exactly what is used.

## Examples

Bad:

```cpp
void MyComponent::Init(Profile* profile) {
  enabled_ = profile->GetPrefs()->GetBoolean(kMyPref);
}

BraveExtensionService::BraveExtensionService(Profile* profile)
    : profile_(profile) {}  // used only for ExtensionSystem::Get(profile_)
```

Good:

```cpp
void MyComponent::Init(PrefService* prefs) {
  enabled_ = prefs->GetBoolean(kMyPref);
}

BraveExtensionService::BraveExtensionService(ExtensionSystem* extension_system)
    : extension_system_(extension_system) {}
```
