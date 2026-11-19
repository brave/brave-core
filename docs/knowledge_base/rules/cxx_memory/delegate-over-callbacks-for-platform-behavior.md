---
name: brave/delegate-over-callbacks-for-platform-behavior
description:
  'A component-level class needing platform-specific behavior takes a dedicated
  Delegate class rather than raw callbacks.'
rubric: design
severity: should_fix
---

**Rule.** When a class under `components/` needs behavior that the embedder or
platform layer provides, define a `Delegate` interface for it and have the class
own or reference a `Delegate` implementation. Do not inject that behavior as
`base::RepeatingCallback` / `base::OnceCallback` members.

**Why.** A delegate gives a cleaner interface, safer lifetime management than a
set of loose callbacks, and is easier to replace with a fake in tests.

## Examples

Bad:

```cpp
class DefaultBrowserMonitor {
  base::RepeatingCallback<bool()> is_default_browser_callback_;
};
```

Good:

```cpp
class DefaultBrowserMonitor {
  class Delegate {
   public:
    virtual bool IsDefaultBrowser() = 0;
  };
  std::unique_ptr<Delegate> delegate_;
};
```
