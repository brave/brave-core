# Service Workers on `chrome-untrusted://` WebUIs

A WebUI's service worker can only be registered from C++, and the WebUI host
needs to opt-in before the worker is allowed to service its navigations.

## How It Works

### 1. Opt-in

On your `WebUIConfig` override `ShouldInterceptNavigationsWithServiceWorker` to
be true. This allows your service worker to serve navigations.

```cpp
virtual bool ShouldInterceptNavigationsWithServiceWorker() {
  return true;
}
```

### 2. Register the service worker

Register the service worker from your WebUI

```cpp
const GURL scope = GURL(kFooUIURL);
blink::mojom::ServiceWorkerRegistrationOptions options(
    scope, blink::mojom::ScriptType::kClassic,
    blink::mojom::ServiceWorkerUpdateViaCache::kNone);

browser_context->GetDefaultStoragePartition()
    ->GetServiceWorkerContext()
    ->RegisterServiceWorker(
        scope.Resolve("sw.js"),
        blink::StorageKey::CreateFirstParty(url::Origin::Create(scope)),
        options, content::GlobalRenderFrameHostId(), std::move(callback));
```

## Restrictions

### Registration is only possible from C++

`navigator.serviceWorker.register()` and `.getRegistrations()` reject with a
`SecurityError`:

```
Failed to register a ServiceWorker: The URL protocol of the current origin
('chrome-untrusted://foo') is not supported.
```

Installing/uninstalling a WebUI service worker can only be done from the browser
process.

### The worker cannot fetch `chrome-untrusted://` resources itself

`fetch` and `importScripts` currently don't work from inside a serviceWorker.

### `chrome-untrusted://` only

Only the untrusted scheme gets a worker script factory. Registering for a
`chrome://` scope fails with `kErrorNetwork`, even if its config opts in.

### Registration and the navigation opt-in are independent

Registration succeeds whether or not the config opts in. But without the opt-in
the navigation keeps the shortcut, so the document will never get a service
worker client and the worker never serves anything for that host.

### An opted-in host leaves the WebUI navigation fast path

There's normally a shortcut for WebUI navigation - opting into service worker
registration means all requests will go through the full navigation stack, so
only opt in WebUIs which need a service worker.
