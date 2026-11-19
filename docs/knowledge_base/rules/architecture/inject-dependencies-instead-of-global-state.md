---
name: brave/inject-dependencies-instead-of-global-state
description:
  'Features take the dependencies they need as constructor parameters, instead
  of reaching for g_browser_process, singletons, NoDestructor globals or a
  globally looked-up Profile.'
rubric: design
severity: should_fix
---

**Rule.** A feature, in any layer, receives what it uses as constructor
parameters. It does not reach out to global state for it: `g_browser_process`,
singletons and their `GetInstance()`, `base::NoDestructor` globals, or a profile
found through `ProfileManager::GetActiveUserProfile()` and similar. When a
feature genuinely needs a lookup key such as a `Profile`, inject the key rather
than pulling it from a global.

**Why.** Construction-time injection makes a feature testable, since tests
substitute its dependencies directly; it surfaces dependency cycles at
construction and BUILD.gn time instead of hiding them behind globals; and it
avoids "spooky action at a distance" between seemingly unrelated features.

## Examples

Bad:

```cpp
void FooFeature::DoStuff() {
  auto factory = g_browser_process->shared_url_loader_factory();
  Profile* profile = ProfileManager::GetActiveUserProfile();
  UseKeyedService(BarServiceFactory::GetForProfile(profile));
}
```

Good:

```cpp
FooFeature(BarService& bar_service,
           scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : bar_service_(bar_service),
      url_loader_factory_(std::move(url_loader_factory)) {}
```
