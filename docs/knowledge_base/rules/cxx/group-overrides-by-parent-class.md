---
name: brave/group-overrides-by-parent-class
description:
  'In a header, overrides are grouped by the parent class they come from, one
  labelled group per parent within each access section.'
rubric: readability
severity: nit
---

**Rule.** Within each access section of a class declaration, keep the overrides
of each parent class together, under a `// ParentClass:` comment naming it. Do
not scatter overrides among other methods or mix those of several parents in one
group.

**Why.** Labelled groups show at a glance which interface each method
implements, and where to add the next override of the same parent.

## Examples

Bad:

```cpp
class BraveAdsService : public KeyedService, public PrefObserver {
 public:
  void OnPrefChanged(const std::string& path) override;
  void Shutdown() override;
};
```

Good:

```cpp
class BraveAdsService : public KeyedService, public PrefObserver {
 public:
  // KeyedService:
  void Shutdown() override;

  // PrefObserver:
  void OnPrefChanged(const std::string& path) override;
};
```
