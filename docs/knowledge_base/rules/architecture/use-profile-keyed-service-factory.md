---
name: brave/use-profile-keyed-service-factory
description:
  'New desktop keyed service factories inherit from ProfileKeyedServiceFactory,
  not BrowserContextKeyedServiceFactory.'
rubric: design
severity: should_fix
---

**Rule.** A new keyed service factory in desktop browser code (`browser/`)
derives from `ProfileKeyedServiceFactory`, and states which profiles get the
service with `ProfileSelections`. Do not derive a new one from
`BrowserContextKeyedServiceFactory`. Factories in `components/`, which cannot
depend on `//chrome`, are not concerned.

**Why.** `ProfileKeyedServiceFactory` is the current base for profile-keyed
services, and its `ProfileSelections` describe the profile types declaratively;
the older base leaves each factory to reimplement that selection.

## Examples

Bad:

```cpp
class FooServiceFactory : public BrowserContextKeyedServiceFactory {
```

Good:

```cpp
class FooServiceFactory : public ProfileKeyedServiceFactory {
```
