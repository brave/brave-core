---
name: brave/service-is-null-while-testing-for-optional-services
description:
  'When a keyed service must not be created in unit tests that lack its
  dependencies, its factory overrides ServiceIsNULLWhileTesting() instead of the
  service null-checking them throughout.'
rubric: design
severity: should_fix
---

**Rule.** When a desktop `KeyedService` cannot work in unit tests that do not
provide its dependencies (local state, a network factory), override
`ServiceIsNULLWhileTesting()` in its factory to return `true`. Do not instead
add "might be null in tests" checks of those dependencies across the service's
methods.

**Why.** The factory then decides once that the service does not exist in such
tests, and the service's code can rely on its dependencies being present.

## Examples

Bad:

```cpp
void MyService::DoSomething() {
  if (!local_state_) {
    return;  // might be null in tests
  }
}
```

Good:

```cpp
bool MyServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
```
