# Which site's Shields settings apply to a request?

The **Shields settings origin** is the key Brave passes to `IsBraveShieldsEnabled`,
`GetAdControlType`, and `GetCosmeticFilteringControlType`. It is an origin URL
(scheme, host, and effective port), or `GURL()` for profile defaults. It answers
whose preferences to read, not whether an adblock rule considers a request
first-party. In the examples below, A, B, and C are distinct HTTPS origins.

## Main-frame navigations

For a primary-main-frame HTTP navigation from `https://a.test/old` to
`https://b.test/new`, use **B** for the HTTP request for `/new`. This applies to
the navigation request itself, not just to requests made by `/new` after commit.
For a main-frame network request with no usable settings origin, use **profile
defaults**. Never substitute the origin of the document being navigated away from.

For a cross-document navigation without redirects, the boundaries are:

- **Start:** In `WebContentsObserver::DidStartNavigation(handle)` for B,
  `handle->IsInPrimaryMainFrame()` is true, `handle->IsSameDocument()` and
  `handle->HasCommitted()` are false, and `handle->GetURL()` identifies B.
  `web_contents->GetPrimaryMainFrame()->GetLastCommittedURL()` still identifies
  A. Starting B's navigation must not change the settings stored for A's document.
- **Request:** Before `ReadyToCommitNavigation(handle)` for B, Brave evaluates
  the network request for `/new`. At the first Shields lookup in `MakeCTX`,
  `ctx->tab_origin()` for **that request** must equal `GURL("https://b.test/")`,
  even if the request's frame token still resolves to A's `RenderFrameHost`.
- **Finish:** If `DidFinishNavigation(handle)` reports `HasCommitted()` and
  `!IsErrorPage()` for B, fetches by the B document use B. If the navigation is
  canceled without committing, fetches by the A document continue to use A.

## Requests made by documents

A document's fetches and WebSockets use the settings of its **outermost
main-frame document** (`RenderFrameHost::GetOutermostMainFrame()`). If A embeds
B, and B embeds C, a fetch from C uses A regardless of the fetch URL. The same
rule applies to a fetch issued by A after a navigation to B starts but before B
commits: that fetch uses A, not B. A subframe navigation does not give the tab
the subframe's settings.

Some top-level documents have a DOM origin that is not their Shields settings
origin. The following cases define which settings those documents have:

| Creation or navigation | Settings for requests from the resulting document |
| --- | --- |
| An HTTP response from A commits with a sandbox policy that makes `GetLastCommittedOrigin().opaque()` true. | A, from the HTTP navigation to A. Never extract the precursor of the opaque origin. |
| A child frame B under A opens a popup. Script in the popup's **initial empty main-frame document** issues a request. | A, from the opening frame's outermost document. |
| B under A opens an inheriting `about:blank` popup. | A, even when the popup's DOM origin is B. The same settings apply when a sandboxed opener yields an opaque popup, including the `noopener` case. |
| An inherited opaque popup opens another inheriting popup. | Copy the first popup's stored settings value. If the first inherited A, both use A. |
| A sandboxed A document navigates itself to `about:blank`, or a sandboxed child B under A initiates an allowed top-frame navigation to `about:blank`. | A, from the initiating frame's outermost document. A context-menu navigation that creates an inheriting tab from that frame likewise uses A. |
| A browser-initiated `data:` navigation has no document from which to inherit. | Profile defaults. A previous document in the tab does not supply a value. |
| B under A opens a popup directly at C, or an inherited popup later navigates to C. | C for the HTTP navigation request and C for the committed C document. |

### Popup creation, navigation, and destruction

Copy the **lookup key**, not the opener's `ALLOW`/`BLOCK` decision. Suppose
`https://a.test/page` embeds `https://b.test/frame`, and B opens popup P. If P
inherits, its key is `https://a.test/`. Every subsequent request from P looks
up the permissions then configured for A. Changing A's Shields preference
therefore changes filtering in P; changing the opener's URL does not.

Establish the key for P's initial empty document at creation, before its first
request is filtered. `DidOpenRequestedURL(new_contents, source_render_frame_host, ...)`
identifies the opening frame; use its **outermost document's stored key**.
Do not consult `window.opener`, save an opener RFH for later lookup, or assume
`DidOpenRequestedURL` precedes every initial-document request. If a first
request can precede that notification, its creation context must carry the
same key. A late creation notification must never overwrite a different
document that has already committed in P. Copy an empty key as defaults too.

After P's inherited `about:blank` document has loaded, apply these transitions:

| Event | Key for the next request made by P's document |
| --- | --- |
| The opener changes its fragment, calls `history.pushState`, or commits a different path on A. | A. None of these events writes P's document state. |
| The opener commits `https://d.test/new`, or its opening iframe B navigates or is removed. | A. Neither D nor the frame now occupying B's position is a source for P. |
| The opener tab closes and its `WebContents` is destroyed. | A, even for the first request P makes **after destruction**. No opener lookup is needed. |
| P changes its own URL with a same-document navigation (`handle->IsSameDocument()`). | A. Keep the document value. |
| P starts a cross-document HTTP navigation to C. | The HTTP request for C uses C. Requests from the still-committed inherited document use A. Cancellation with `!handle->HasCommitted()` leaves A in place. After a successful commit, requests from the C document use C. |
| P opens an inheriting popup Q, then P closes or navigates to C. | Q keeps A. Each generation copies the key at creation; Q has no dependency on either ancestor. |

Store a committed top-level document's key on that document and on its
committed `NavigationEntry`. Going Back from C to P's inherited entry restores
A whether the opener is on D or has closed. On BFCache activation keep the
retained document's value; on document recreation restore the entry's saved
value (including a saved default). Do not infer it from the `about:blank` URL,
the document at C, the current opener, or a reused `RenderFrameHost`.

For an inheriting cross-document navigation, capture the initiating outermost
document's key on the `NavigationHandle` at `DidStartNavigation`, while the
initiator is identifiable. If there is no identifiable source or saved value,
use defaults. A navigation that supplies its own site does not inherit merely
because it has an initiator. Only install a new document value on a pending
commit host belonging to that navigation or at commit; never install it on a
host that still serves the previous document. Discard uncommitted navigation
state on cancellation.

## Worker requests

Dedicated, shared, and service worker requests must retain the top-level
settings context available to their loader factory. For a worker used by B
under A, use A for fetches and WebSockets when A is the factory's top-frame
origin. When no frame token or request-level trusted isolation info is available,
use the factory's top-frame isolation origin if usable; if opaque or absent,
use defaults. Do not use the worker script's origin as a substitute. A WebSocket
with unset `resource_type` is not a main-frame navigation merely because the
default numeric value of that field denotes `kMainFrame`.

## Implementation constraints

- `ShieldsSettingsOrigin` owns the selection rules and the result. Callers must
  not assemble candidate origins or write navigation, document, or history
  storage. Lifecycle writes belong on `BraveShieldsWebContentsObserver`.
- `BraveRequestInfo::shields_settings_origin_` is a snapshot for **one request**.
  At the first settings lookup its `GetURL()` must already be the key specified
  above. `GetShieldsSettingsOrigin` and `tab_origin()` may read it; neither may
  calculate it. An empty value means use defaults, not try another source.
- Do not change the adblock engine, `request_initiator_for_adblock_rules()`, or
  `first_party_origin_for_adblock_rules()`. Do not add a universal observer,
  renderer plumbing, or keepalive throttles.

### Handoff before the first network settings lookup

Use the browser-side request creation boundary, before `MakeCTX` reads content
settings. `BraveProxyingURLLoaderFactory::InProgressRequest` and
`BraveProxyingWebSocket` each have a `CreateBraveRequestInfo()` path. The
request owner supplies a typed `ShieldsSettingsOrigin` snapshot to `MakeCTX`
on both ownership variants. `MakeCTX` assigns that value directly to
`shields_settings_origin_`. Recreating a context for the same request stage
must not re-read a now-navigated frame or opener.

The following rules specify the snapshot's source. Implement them inside
`ShieldsSettingsOrigin`, using the existing request/factory context; the proxy
must not assemble a list of candidate origins:

1. Use the request's trusted `IsolationInfo` when present. Only if absent may
   `kWorkerMainResource`, `kWorkerSubResource`, `kServiceWorkerScript`, or
   `kServiceWorkerSubResource` use factory isolation info. Determine whether
   this is a main-frame navigation from the isolation request type when
   available; otherwise use the request resource type. A WebSocket handshake
   is not a navigation.
2. For the initial main-frame network request, Chromium supplies the navigation
   isolation context on the request. For `kMainFrame`, snapshot its non-opaque
   `top_frame_origin()` as the settings key; missing or opaque means defaults.
   Do not read a frame or infer the key from the URL to override opacity. This
   source exists before the response and is independent of the document that
   will later commit. The server-observed navigation test below must verify
   this handoff on a real request; the unit fixture alone cannot establish it.
3. For a framed subresource or subframe navigation, snapshot the owning
   outermost document's key. Without stored document data, use its non-opaque
   committed origin, or its committed HTTP(S) URL's origin for a sandboxed
   HTTP(S) document; otherwise use defaults. Never override a **stored**
   default. For a frameless request use the eligible isolation context's
   non-opaque top-frame origin, otherwise defaults. The request initiator,
   opener's current URL, and opaque precursor are not substitutes.

## Acceptance tests

Use separate browser tests for distinct lifecycle scenarios. Give the expected
site and the competing site opposite settings, reverse them, and include a
successful unfiltered control so a failed request alone cannot pass the test.

- **Navigation:** Have B's server report the path and query it actually received
  for `/new`. With a rule that strips a query parameter, the parameter must be
  stripped when filtering is enabled on B only and retained when enabled on A
  only. A successful fetch from B after commit does not test this lookup.
- **Initial popup:** Issue the measured request from the initial empty main-frame
  document before a later document commits. A fetch from a child added to the
  popup or from a committed `about:blank` document tests a different case.
- **Opener URL change after load:** Wait for P's inherited document to load,
  commit D in the opener, then fetch in P. Allow only A and expect success;
  allow only D and expect blocking. Keep P on the same document throughout.
- **Opener closure after load:** In a separate test, wait for P to load, close
  the opener, and wait for destruction of its `WebContents`. From P, fetch an
  unfiltered control, then issue the filtered request with A allowed and with
  A blocked. It must follow A in both cases, with defaults set oppositely.
- **History:** Verify destruction of the original document, navigate the opener
  elsewhere, and observe filtering after Back restores the same entry.
- **Unit tests:** Keep input/output tests for selection and precedence, including
  missing or opaque isolation origins. Use browser tests when the claim is that
  Chromium generates a particular request or delivers a callback at a particular
  time; do not supply that input or invoke that callback from the test.

## Regression checklist

For each change to origin selection, check the expected result in each row.
Run `AdBlockServiceSettingsOriginTest.*` in `brave_browser_tests` on Linux;
individual tests have platform guards. Test names below are relative to that
suite in [the browser test source](../browser/brave_shields/ad_block_service_browsertest.cc).
The last Linux run passed 28 of 30; the two unverified rows are requirements,
not evidence that the branch satisfies them.

| Scenario and expected result | Browser test |
| --- | --- |
| Navigate A → B: filtering the initial HTTP request must follow B, not A. **Unverified:** the filtered navigation failed before the server-echo assertion. | `MainFrameNavigationUsesDestinationSettings` |
| Fetch from child B under A: use A. | `SubFrameUsesTopFrameSettings` |
| Fetch from a nested cross-site frame: use outermost A. | `NestedFrameUsesTopFrameSettings` |
| Sandboxed HTTP document with opaque DOM origin: use its HTTP site's settings. | `SandboxedMainFrameUsesUrlSettings` |
| Sandboxed A navigates itself to `about:blank`: retain A despite the opaque committed origin. | `SandboxedPageNavigatesItselfToAboutBlank` |
| Sandboxed B under A navigates the top frame to `about:blank`: use A, not B or defaults. | `SandboxedChildNavigatesTopToAboutBlank` |
| B under A calls `window.open()`: a request from the initial empty document must use A. **Unverified:** the initial-blank assertion failed; the test also attempts its fetch from a child rather than from that document. | `InitialEmptyPopupUsesOpenerPageSettings` |
| B under A opens `about:blank`: the popup itself fetches using A even though its DOM origin is B. | `AboutBlankPopupFromSubFrameUsesOpenerPageSettings` |
| C embedded in that popup fetches: use A, not opener frame B or child C. | `FrameInAboutBlankPopupFromSubFrameUsesOpenerPageSettings` |
| Load a child frame at C in that popup: the `$subdocument` rule for the **frame navigation itself** follows A. | `FrameNavigationInAboutBlankPopupFromSubFrameUsesOpenerPageSettings` |
| B under A opens directly at C: a fetch after commit uses C. | `CrossSitePopupFromSubFrameUsesPopupSettings` |
| Navigate an already inherited popup to C: a fetch after commit uses C instead of A or B. | `InheritedPopupSwitchesToCommittedSiteSettings` |
| Fetch from a child of a popup opened by a sandboxed frame: follow the opening page A. | `OpaquePopupUsesOpenerPageSettings` |
| Repeat the opaque-popup child fetch with `noopener`: still use A. | `OpaqueNoopenerPopupUsesOpenerPageSettings` |
| Open an inheriting popup from an opaque popup: preserve the original page's settings through both openings. | `OpaquePopupFromOpaquePopupUsesOriginalPageSettings` |
| Navigate the opener from A to D: requests in the popup continue to follow A. | `OpaquePopupKeepsSettingsAfterOpenerNavigates` |
| Change the inherited popup's fragment: retain the same document and A's settings. | `InheritedPopupKeepsSettingsAcrossFragmentNavigation` |
| Destroy the inherited document, navigate its opener elsewhere, then go Back: the recreated document uses the original entry's settings. | `RecreatedInheritedPageKeepsSettingsOnBack` |
| Use Open Link in New Tab on `about:blank` in a sandboxed frame: the new tab uses A (desktop context-menu path). | `ContextMenuTabFromSandboxedFrameUsesPageSettings` |
| Browser navigates A to `data:`; a fetch from child B follows defaults, neither A nor B. | `BrowserInitiatedOpaqueTopFrameUsesDefaultSettings` |
| Browser navigates a site popup to `data:`: its own fetch follows defaults, not the opener. | `BrowserInitiatedOpaquePopupUsesDefaultSettings` |
| WebSocket opened by B under A: use A. | `WebSocketInSubFrameUsesTopFrameSettings` |
| Dedicated worker fetch from B under A: use A. | `DedicatedWorkerInSubFrameUsesTopFrameSettings` |
| Dedicated worker WebSocket from B under A: use A. | `DedicatedWorkerWebSocketInSubFrameUsesTopFrameSettings` |
| Shared worker fetch from B under A: use A. | `SharedWorkerInSubFrameUsesTopFrameSettings` |
| Shared worker WebSocket from B under A: use A. | `SharedWorkerWebSocketUsesTopFrameSettings` |
| Service worker fetch from B under A: use A. | `ServiceWorkerInSubFrameUsesTopFrameSettings` |
| Service worker WebSocket from B under A: use A. | `ServiceWorkerWebSocketInSubFrameUsesTopFrameSettings` |
| Shared worker fetch from B embedded in a `data:` top frame: use defaults, not B. | `SharedWorkerInOpaqueTopFrameUsesDefaultSettings` |
| Shared worker WebSocket from B embedded in a `data:` top frame: use defaults, not B. | `SharedWorkerWebSocketInOpaqueTopFrameUsesDefaultSettings` |

### Supplied-input and compatibility checks

Run `ShieldsSettingsOriginTest.*:BraveRequestInfo*Test.*` in `brave_unit_tests`.
The tests below identify inputs that a browser test cannot reliably force. They
also preserve the distinction between the settings URL and the origins used by
adblock rules. Names refer to [the frame fallback test](../browser/brave_shields/shields_settings_origin_unittest.cc)
and [request context tests](../browser/net/url_context_unittest.cc).

| Inputs or invariant | Unit tests |
| --- | --- |
| No stored document value or Brave observer: use the committed non-opaque frame origin. | `GetForFrameWithoutValueUsesCommittedOrigin` |
| Child request without isolation info: preserve the outermost top-frame origin for rules; if the page has an inherited settings value, use it for settings. | `WithoutIsolationInfoUsesOutermostMainFrameOrigin`, `WithoutIsolationInfoUsesStoredPageValue` |
| For both `kSubFrame` and `kOther`, a live frame's stored settings value wins over conflicting isolation info. | `FrameRequestUsesStoredPageValue` |
| For `kMainFrame`, use a non-opaque top-frame isolation origin even with an outgoing frame token. An opaque one gives defaults whether the outgoing document has inherited site settings or no site settings. | `MainFrameNavigationUsesNonOpaqueTopFrameOrigin`, `MainFrameNavigationWithOpaqueTopFrameUsesDefaults`, `MainFrameNavigationWithOpaqueStoredPageValueHasNoTabOrigin` |
| Frameless worker: omitted request context permits worker factory fallback; explicit request context wins. A document-subresource factory must not supply this fallback. | `UsesFactoryContextWhenWorkerRequestOmitsRequestContext`, `RequestContextOverridesFactoryContext`, `NonWorkerFactoryTypeIgnoresFactoryContextFallback` |
| Frameless WebSocket: a request initiator alone does not supply a settings URL; neither does an opaque top-frame isolation origin. | `FramelessRequestIgnoresInitiatorForTabOrigin`, `FramelessRequestWithOpaqueTopFrameHasNoTabOrigin` |
| Initiator bookkeeping is independent: use the worker factory initiator when omitted; preserve an original initiator over a replacement request initiator. | `WorkerRequestFallsBackToFactoryInitiator`, `OriginalInitiatorOverridesRequestInitiator` |
| Preserve raw opaque top-frame origins with and without precursors. Preserve the separate rule inputs for missing, non-opaque, and opaque initiators and for main-frame, subframe, and subresource requests. These are not settings-origin fallback rules. | `KeepsOpaqueTopFrameOrigin`, `KeepsOpaqueTopFrameOriginWithoutPrecursor`, `RequestInitiatorForAdblockRules*`, `FirstPartyOriginForAdblockRules*` |

The browser checklist does not establish opener closure, opening-frame removal,
changes to the opener via the same-document History API, cancellation before
commit, or restoration of a *retained* BFCache document. Opener navigation and
history recreation have dedicated tests; neither substitutes for those cases.
