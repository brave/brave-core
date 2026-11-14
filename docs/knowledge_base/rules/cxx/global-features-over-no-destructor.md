---
name: brave/global-features-over-no-destructor
description:
  'A global or singleton service is registered in GlobalFeatures rather than
  held in a base::NoDestructor static.'
rubric: design
severity: should_fix
---

**Rule.** When a change adds a browser-wide service, register it in
`GlobalFeatures`, which replaces `BrowserProcessImpl` for this, and reach it
through `g_browser_process` or dependency injection. Do not implement it as a
singleton returned from a `static base::NoDestructor` in `GetInstance()`.

**Why.** A `NoDestructor` instance lives until the process exits: tests cannot
reset or replace it, so state leaks from one test to the next.

## Examples

Bad:

```cpp
BraveOriginService* BraveOriginService::GetInstance() {
  static base::NoDestructor<BraveOriginService> instance;
  return instance.get();
}
```

Good:

```cpp
// Created and owned by GlobalFeatures; tests can replace it.
BraveOriginService* service =
    g_browser_process->GetFeatures()->brave_origin_service();
```
