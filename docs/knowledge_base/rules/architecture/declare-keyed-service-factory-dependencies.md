---
name: brave/declare-keyed-service-factory-dependencies
description:
  'A keyed service factory whose service uses other keyed services declares each
  of their factories with DependsOn() in its constructor.'
rubric: correctness
severity: should_fix
---

**Rule.** When a keyed service factory builds a service that uses another keyed
service (it calls `OtherServiceFactory::GetForProfile()` or similar while
building or running it), the factory's constructor MUST call
`DependsOn(OtherServiceFactory::GetInstance())` for each of them.

**Why.** `DependsOn` tells the dependency manager the order in which to create
and destroy the services. Without it a service can be built before what it uses
exists, or outlive it at shutdown and touch a destroyed service.

## Examples

Bad:

```cpp
MyServiceFactory::MyServiceFactory()
    : ProfileKeyedServiceFactory("MyService") {}

std::unique_ptr<KeyedService>
MyServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<MyService>(RewardsServiceFactory::GetForProfile(
      Profile::FromBrowserContext(context)));
}
```

Good:

```cpp
MyServiceFactory::MyServiceFactory()
    : ProfileKeyedServiceFactory("MyService") {
  DependsOn(RewardsServiceFactory::GetInstance());
}
```
