# Network Interception Best Practices

Apply these practices to new features that need to inspect, block, redirect, or
modify network requests or responses. The architectural direction comes from
[brave-browser#57532](https://github.com/brave/brave-browser/issues/57532):
replace the complex shared network proxies with feature-specific Chromium
integration. The issue's
[migration proposal](https://github.com/brave/brave-browser/issues/57532#issuecomment-6068654412)
describes supporting work; its proposed classes and loader capabilities are not
existing APIs.

<a id="NET-001"></a>

## Choose the Narrowest Supported Integration

**Do not add new `*_network_delegate_helper` callbacks or extend
`BraveProxyingURLLoaderFactory`, `BraveProxyingWebSocket`, or their shared
dispatcher for new features.** Do not replace them with another general-purpose
interception framework. Reuse upstream behavior first, then choose an
integration that supports the feature's required stage and request types.

| Requirement                                                     | Preferred integration                                                                                                              |
| --------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| Existing Chromium capability                                    | Reuse it, preserving Brave's settings and policy semantics.                                                                        |
| Navigation decisions, consent, or interstitials                 | `content::NavigationThrottle`. This does not cover subresource requests.                                                           |
| Resource request checks, header changes, or response processing | A feature-specific `blink::URLLoaderThrottle`, in each process that handles the relevant requests.                                 |
| Renderer-local subresource filtering                            | Rulesets consumed by `DocumentSubresourceFilter`, only when its actions, exceptions, and precedence represent the required policy. |
| Requests constructed by a browser-owned service                 | Apply policy at request construction when that covers the feature.                                                                 |
| Secrets, signing, or network-stack state                        | A narrowly scoped browser/network-service integration at the stage that owns the operation.                                        |
| WebSocket connection policy                                     | A browser-side connection decision before invoking the network factory; ordinary URL-loader throttles do not cover WebSockets.     |

For URL-loader throttles, existing entry points include
`BraveContentBrowserClient::CreateURLLoaderThrottles()` and
`URLLoaderThrottleProviderImpl::CreateThrottles()`, reached through
`BraveContentRendererClient::CreateURLLoaderThrottleProvider()`. Preserve
upstream throttles when adding Brave throttles. Audit keepalive and prefetch
separately: Chromium exposes `CreateURLLoaderThrottlesForKeepAlive()` and
`ModifyRequestHeadersForPrefetch()`; ordinary browser throttle registration does
not establish coverage of those paths.

If no existing API supports the required behavior, identify the missing
capability explicitly and add narrowly scoped support before enabling the
feature. Follow the [Chromium override practices](chromium-src-overrides.md).

<a id="NET-003"></a>

## Respect the Throttle Contract

Check the checked-out Chromium
[`URLLoaderThrottle` contract](https://chromium.googlesource.com/chromium/src/+/main/third_party/blink/public/common/loader/url_loader_throttle.h)
and its caller before selecting an implementation.

- Modify `ResourceRequest` before `WillStartRequest()` returns. Deferral does
  not permit retaining and asynchronously mutating its pointer. Copy inputs
  needed by asynchronous work.
- Apply redirect header changes through `HttpRequestHeadersUpdateParams`.
  Reevaluate destination eligibility and remove feature-added headers when a
  redirect leaves their allowed scope.
- Use one coordinated owner for URL rewriting. Multiple throttles cannot
  independently change the URL at the same stage; redirect URL changes also have
  same-origin restrictions. Preserve redirect/security checks, method and upload
  semantics, and loop limits.
- Defer before network contact when a decision must prevent contact. Complete
  asynchronous work through resume or cancellation, using weak callbacks and
  explicit handling for service/pipe disconnection and context destruction.
  Define the feature's failure behavior so requests cannot remain deferred.
  Deferring one throttle does not stop later throttles from seeing that stage.
- Make changes safe when applied again, including service-worker fallback that
  creates a new throttle chain. Avoid duplicate headers, evaluation, and
  reporting without trusting renderer-controlled bypass markers.

Do not assume the delegate can apply every asynchronous result. Stock
`URLLoaderThrottle` has no arbitrary asynchronous URL setter or pre-network
substitute-response API. `CancelWithError()` alone does not expose element
collapse status. Define and implement the required support for rewrites,
substitute resources, or specialized completion results before using those
outcomes.

<a id="NET-004"></a>

## Preserve Header and Response Semantics

Custom renderer-added headers require explicit CORS treatment. If a header must
be CORS-exempt, register only that exact header in the network context's
`cors_exempt_header_list`, preserve existing entries, and use
`cors_exempt_headers` and redirect `modified_cors_exempt_headers`. Test that the
feature neither introduces unintended preflights nor sends headers to newly
eligible destinations. Preserve explicitly supplied values where required.

Response processing must keep raw headers and `URLResponseHead::parsed_headers`
consistent, including regenerating parsed headers after CSP changes. Deferred
response replacement must preserve body ownership;
`UpdateDeferredResponseHead()` transfers a body as well as a response head.
Check composition with MIME sniffing and other body-processing throttles.

Removing a security header from the client-visible response does not prove the
network stack has ignored it as persistent state. Choose the hook according to
which behavior is required. For WebSockets, gate the browser's factory call when
blocking must prevent contact: Blink's `WebSocketHandshakeThrottle` runs after
the connection attempt has begun. Audit handshake headers separately.

<a id="NET-005"></a>

## Prove Coverage and Preserve Behavior

Before shipping, document which request classes need the feature and where each
is enforced. Test relevant navigations and subresources, frame and worker
fetches, service-worker fallback, keepalive, prefetch, downloads, browser/system
requests, WebSockets, and redirects. A navigation test alone does not establish
coverage. Include private profiles, site exceptions, dynamic policy changes, and
teardown while a decision is pending.

Assert observable behavior: server-received headers and URLs, absence of contact
with blocked endpoints, delivered response/CSP behavior, and all supported
decision outcomes. Run formatting, presubmit, and relevant unit and browser
tests for implementation changes.

When replacing an existing interception path, cut over complete request classes
only after every applicable outcome is supported. Keep behavior changes separate
from transport refactoring and remove obsolete proxy handling once coverage is
proven.
