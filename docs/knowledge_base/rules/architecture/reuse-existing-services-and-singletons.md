---
name: brave/reuse-existing-services-and-singletons
description:
  'Before adding a service or singleton, check for an existing one with the same
  purpose and reuse it.'
rubric: design
severity: should_fix
---

**Rule.** When a change adds a service, a keyed service or a singleton, check
whether one already serves the same purpose, and use it rather than adding a
duplicate.

**Why.** Two services for the same thing hold diverging state and double the
code to maintain.

## Examples

Bad:

```cpp
// FooLocaleHelper already provides the locale.
class BarLocaleHelper {
 public:
  static BarLocaleHelper* GetInstance();
  std::string GetLocale() const;
};
```

Good:

```cpp
const std::string locale = FooLocaleHelper::GetInstance()->GetLocale();
```
