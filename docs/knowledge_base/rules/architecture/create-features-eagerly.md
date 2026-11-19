---
name: brave/create-features-eagerly
description:
  'Features are created eagerly with an explicit lifetime
  (ServiceIsCreatedWithBrowserContext, BraveTabFeatures::Init), not lazily on
  first use.'
rubric: design
severity: should_fix
---

**Rule.** Create a feature when its owner is created: a profile-scoped service
with `ServiceIsCreatedWithBrowserContext()` returning `true`, a tab or window
feature in `BraveTabFeatures::Init()` or `BrowserWindowFeatures::Init()`. Do not
create it on first use, in an accessor that builds the object when a member is
still null or a `GetForProfile()` that creates the service on its first call.

**Why.** A lazily created feature comes into existence at whatever moment
something first asks for it, so its initialization order differs between
production and tests (which may never ask), and bugs that depend on it are hard
to reproduce.

## Examples

Bad:

```cpp
Foo* Owner::GetFoo() {
  if (!foo_) {
    foo_ = std::make_unique<Foo>(profile_);
  }
  return foo_.get();
}
```

Good:

```cpp
class MyServiceFactory : public ProfileKeyedServiceFactory {
  bool ServiceIsCreatedWithBrowserContext() const override {
    return true;  // Created when profile is created
  }
};
```
