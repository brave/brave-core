# Network Interception Best Practices

Use this guide to choose an integration for features that inspect, block,
redirect, or modify network requests or responses.

<a id="NET-001"></a>

## Choose the Narrowest Supported Integration

**Do not add new `*_network_delegate_helper` callbacks or extend
`BraveProxyingURLLoaderFactory`, `BraveProxyingWebSocket`, or their shared
dispatcher for new features.** Do not replace them with another general-purpose
interception framework. Reuse upstream behavior first, then choose an
integration that supports the feature's required stage and request types.

| Requirement                                                     | Preferred integration                                                                                                          |
| --------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| Existing Chromium capability                                    | Reuse the upstream implementation.                                                                                             |
| Navigation decisions, consent, or interstitials                 | `content::NavigationThrottle`. This does not cover subresource requests.                                                       |
| Resource request checks, header changes, or response processing | A feature-specific `blink::URLLoaderThrottle`, in each process that handles the relevant requests.                             |
| Renderer-local subresource filtering                            | Rulesets consumed by `DocumentSubresourceFilter`, when its supported actions and rule semantics cover the feature.             |
| Requests constructed by a browser-owned service                 | Apply the behavior at request construction when that covers the feature.                                                       |
| Secrets, signing, or network-stack state                        | A narrowly scoped browser/network-service integration at the stage that owns the operation.                                    |
| WebSocket connection policy                                     | A browser-side connection decision before invoking the network factory; ordinary URL-loader throttles do not cover WebSockets. |

Choose based on the traffic the feature must cover. A navigation-only
integration is insufficient for subresources or worker fetches. Keepalive,
prefetch, downloads, browser/system requests, and WebSockets can require
separate integration points.
