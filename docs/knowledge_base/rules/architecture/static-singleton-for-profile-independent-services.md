---
name: brave/static-singleton-for-profile-independent-services
description:
  'A service with no per-profile state or profile dependency is a static
  base::NoDestructor singleton, not a KeyedService.'
rubric: design
severity: should_fix
---

**Rule.** When a new service keeps no per-profile state and uses no
profile-specific data, make it a static singleton held in a
`base::NoDestructor`. Do not wrap it in a `KeyedService` and a keyed service
factory.

**Why.** A keyed service creates one instance per profile and a factory to
manage them, which is needless complexity, and duplicated state, for data that
is the same for every profile.

## Examples

Bad:

```cpp
class ModelListServiceFactory : public ProfileKeyedServiceFactory { ... };
```

Good:

```cpp
class ModelListService {
 public:
  static ModelListService& GetInstance() {
    static base::NoDestructor<ModelListService> instance;
    return *instance;
  }
};
```
