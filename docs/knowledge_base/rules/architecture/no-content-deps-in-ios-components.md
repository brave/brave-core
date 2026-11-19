---
name: brave/no-content-deps-in-ios-components
description:
  'A component that builds for iOS must not depend on //content or content-layer
  types such as content::WebContents and content::BrowserContext.'
rubric: design
severity: blocking
---

**Rule.** Code of a component that builds for iOS (such as `brave_wallet`),
outside its `content/` subdirectory, MUST NOT include `content/` headers, depend
on `//content` targets, or use content-layer types such as
`content::WebContents` and `content::BrowserContext`. Pass the specific
dependencies instead: a `PrefService*`, a `SharedURLLoaderFactory`.

**Why.** iOS uses WebKit, not Chromium's content layer, so iOS builds have no
`//content`: a content dependency breaks the component's iOS build.

## Examples

Bad:

```cpp
// components/brave_wallet/browser/json_rpc_service.h
JsonRpcService(content::BrowserContext* context, ...);
```

Good:

```cpp
// components/brave_wallet/browser/json_rpc_service.h
JsonRpcService(scoped_refptr<network::SharedURLLoaderFactory> loader_factory,
               PrefService* prefs, ...);
```
