---
name: brave/no-browser-context-in-component-services
description:
  'Component-level services take their specific dependencies (PrefService*, a
  URLLoaderFactory) rather than a content::BrowserContext.'
rubric: design
severity: should_fix
---

**Rule.** A service under `components/` takes the specific dependencies it uses,
such as a `PrefService*` or a `scoped_refptr<network::SharedURLLoaderFactory>`,
as constructor parameters. It does not take a `content::BrowserContext` and
fetch them from it.

**Why.** A `BrowserContext` parameter ties the component to the content layer,
which iOS does not have, so the service cannot be reused there.

## Examples

Bad:

```cpp
explicit MyFeatureService(content::BrowserContext* context);
```

Good:

```cpp
MyFeatureService(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
```
