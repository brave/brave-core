# Network Interception Best Practices

Use this guide to choose an integration for features that inspect, block,
redirect, or modify network requests or responses.

<a id="NET-001"></a>

## ✅ Use Existing Supported Chromium Interception Mechanisms

**Do not add new `*_network_delegate_helper` callbacks or add functionality to
existing network-delegate helpers. Do not extend
`BraveProxyingURLLoaderFactory`, `BraveProxyingWebSocket`, or their shared
dispatcher for new features.** Do not replace them with another general-purpose
interception framework. Reuse upstream behavior first, then choose an
integration that supports the feature's required stage and request types.

| Requirement                                                        | Preferred integration                                                                                                                                    |
| ------------------------------------------------------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Existing Chromium capability                                       | Reuse the upstream implementation.                                                                                                                       |
| Navigation decisions, consent, or interstitials                    | `content::NavigationThrottle`. This does not cover subresource requests.                                                                                 |
| Resource request filtering, header changes, or response processing | A feature-specific `blink::URLLoaderThrottle`, in each process that handles the relevant requests.                                                       |
| Renderer-local subresource filtering                               | Rulesets consumed by `DocumentSubresourceFilter` when they fit the use case; otherwise use `blink::URLLoaderThrottle` for browser or renderer filtering. |
| Requests constructed by a browser-owned service                    | Apply the behavior at request construction when that covers the feature.                                                                                 |
| WebSocket connection policy                                        | A browser-side connection decision before invoking the network factory; ordinary URL-loader throttles do not cover WebSockets.                           |

**Examples**

| Need                                                | Bad choice                                               | Good choice                                       |
| --------------------------------------------------- | -------------------------------------------------------- | ------------------------------------------------- |
| Require consent before navigation                   | Add consent handling to `BraveProxyingURLLoaderFactory`. | `content::NavigationThrottle`.                    |
| Change a resource request header                    | Add functionality to a network-delegate helper.          | `blink::URLLoaderThrottle`.                       |
| Block subresources using compatible filtering rules | Add blocking to `BraveProxyingURLLoaderFactory`.         | Rulesets consumed by `DocumentSubresourceFilter`. |

Choose based on the traffic the feature must cover. A navigation-only
integration is insufficient for subresources or worker fetches. Keepalive,
prefetch, downloads, browser/system requests, and WebSockets can require
separate integration points.

---
