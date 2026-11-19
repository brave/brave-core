---
name: brave/no-g-browser-process-in-components
description:
  'Code in components/ must never use g_browser_process or
  g_brave_browser_process; the dependency is injected through the constructor,
  an Init() method or a callback.'
rubric: design
severity: blocking
---

**Rule.** NEVER use `g_browser_process` (or Brave's `g_brave_browser_process`)
in code under `components/`. Pass what the code needs from it, such as a
`PrefService*` for local state or a `scoped_refptr<SharedURLLoaderFactory>`,
through the constructor, an `Init()` method or a callback.

**Why.** `g_browser_process` belongs to the browser layer, so using it from a
component is a layering violation: the component depends on embedder code and
can no longer be built or tested without it.

## Examples

Bad:

```cpp
// components/p3a/p3a_service.cc
void P3AService::Init() {
  url_loader_factory_ = g_browser_process->shared_url_loader_factory();
}
```

Good:

```cpp
// components/p3a/p3a_service.cc
void P3AService::Init(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  url_loader_factory_ = url_loader_factory;
}
```
