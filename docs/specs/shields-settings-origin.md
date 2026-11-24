# Which site's Shields settings apply to a request?

The **Shields settings origin** is the key Brave passes to `IsBraveShieldsEnabled`,
`GetAdControlType`, and `GetCosmeticFilteringControlType`. It is an origin URL
(scheme, host, and effective port), or `GURL()` for profile defaults. It answers
whose preferences to read. It does not decide whether an adblock rule considers
a request first-party, nor which site to use when evaluating adblock rules. In
the examples below, A, B, and C are distinct HTTPS origins.

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

### Redirects

When a main-frame navigation goes from A to B and B redirects to C, the
request to B uses B and the request to C uses C. Re-evaluate the destination
at each redirect, including a redirect back to B; a same-origin hop keeps the
same key. For a fetch or an embedded-frame navigation owned by A, all hops
use A. A worker request that starts with the profile-default key keeps it.
If Back results in a server redirect, the newly committed destination uses
its own key rather than the one saved on the old entry.

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

## History and session restore

When restoring the original entry without a server redirect, use the same Shields
settings key it had before, whether restored by Back/Forward or by session
restore after browser shutdown. When the key is a site origin, persist that
origin in the navigation entry's session data. When the document used profile
defaults, store no site origin and continue to use defaults on restore. The
key must be available by the first Shields lookup for the restored document;
look up the current preferences rather than restoring an old filtering decision.

## Worker requests

Dedicated, shared, and service worker requests must retain the top-level
settings context available to their loader factory. For a worker used by B
under A, use A for fetches and WebSockets when A is the factory's top-frame
origin. When no frame token or request-level trusted isolation info is available,
use the factory's top-frame isolation origin if usable; if opaque or absent,
use defaults. Do not use the worker script's origin as a substitute.

## Implementation constraints

- `ShieldsSettingsOrigin` owns the selection rules and the result. Callers must
  not assemble candidate origins or write navigation, document, or history
  storage. Lifecycle writes belong on `BraveShieldsWebContentsObserver`.
- `BraveRequestInfo::shields_settings_origin_` is a snapshot for **one request stage**.
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
must not re-read a now-navigated frame or opener. On a redirect, pass the
updated request to `ShieldsSettingsOrigin::ForRedirect` before constructing
the next stage’s context.

The following rules specify the snapshot's source. Implement them inside
`ShieldsSettingsOrigin`, using the existing request/factory context; the proxy
must not assemble a list of candidate origins:

1. Use the request's trusted `IsolationInfo` when present. Only if absent may
   `kWorkerMainResource`, `kWorkerSubResource`, `kServiceWorkerScript`, or
   `kServiceWorkerSubResource` use factory isolation info. Determine whether
   this is a main-frame navigation only from the isolation request type. The
   resource type is not a source: a navigation request that a service worker
   fetches keeps `kMainFrame`, and a WebSocket handshake leaves it unset,
   which is numerically `kMainFrame`. A nonce on main-frame isolation info,
   such as the one for a site whose storage is forgotten on close, does not
   change whether it is a main-frame navigation.
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
- **Redirects:** For a main-frame navigation, a document fetch, and an embedded
  frame navigation, observe what each server actually receives. Reverse the
  preferences and verify every hop, including the return to the first host.
  Separately verify the default-key worker request and the document committed
  after a history redirect.
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
- **Session restore:** Close the opener of an inherited `about:blank` popup,
  shut down the browser, and restore the session. Verify that a request from
  the restored popup follows the original site's current settings. Repeat for
  a popup that inherited profile defaults.
- **Unit tests:** Keep input/output tests for selection and precedence, including
  missing or opaque isolation origins. Use browser tests when the claim is that
  Chromium generates a particular request or delivers a callback at a particular
  time; do not supply that input or invoke that callback from the test.

## Required browser tests

Each row below is a separate test. Apply the rules in
[Acceptance tests](#acceptance-tests) to every row: set the expected key and
each competing key to opposite settings, reverse them, and include a successful
control. "Defaults" means the profile default settings. A, B, C, D, and E are
distinct sites; P is a popup, and Q is a popup opened by P.

### Navigations and redirects

| Setup | Measured request | Expected key |
| --- | --- | --- |
| Main frame on A navigates to B. | B's server records the request for `/new`. | B |
| Main frame on A navigates to B, whose cookies are cleared when the site closes, so its isolation info carries a nonce. | B's server records the request for `/new`. | B, not A |
| Main frame on B redirects to C, and C redirects back to B. | Each server records the request for its hop. | The destination of each hop: B, then C, then B |
| A document on A fetches a URL that redirects B → C → B. | Each server records the request for its hop. | A for every hop |
| A on A loads a child frame through B → C → B. | Each server records the request for its hop. | A for every hop |
| A on A loads a script whose URL redirects from B to C. A block rule matches C's URL. | The script request to C | A, not B or C |
| Main frame on A navigates to B, which redirects to C. A `$csp` rule matches C. | The CSP added to C's response | C, not A or B |
| A on A loads a child frame that redirects from B to C. A `$csp` rule matches C. | The CSP added to C's response | A, not B or C |
| A `data:` top frame embeds B. B's shared worker fetches a URL that redirects from B to C. | The final request to C | Defaults |
| A tab visits B, then another site. Going Back to B redirects to C; BFCache is disabled. | A fetch from the committed C document | C, not B or the other site |
| Main frame on A starts a cross-site navigation that has not committed. | A fetch from the outgoing A document | A |

### Frames and sandboxed documents

| Setup | Measured request | Expected key |
| --- | --- | --- |
| A embeds B. | A fetch from B | A |
| A embeds B, which embeds C. | A fetch from C | A |
| An HTTP response from A commits a sandboxed main document with an opaque origin. | A fetch from that document | A |
| A sandboxed A document navigates itself to `about:blank`. | A fetch from the `about:blank` document | A |
| Sandboxed B under A navigates the top frame to `about:blank`. | A fetch from the top-level `about:blank` document | A, not B or defaults |
| From a sandboxed frame under A, use Open Link in New Tab on `about:blank` (desktop only). | A fetch from the new tab | A |
| The browser navigates a tab from A to `data:`, which then embeds B. | A fetch from B | Defaults, not A or B |

### Popup creation

| Setup | Measured request | Expected key |
| --- | --- | --- |
| B under A calls `window.open()`. | A fetch from the popup's initial empty document, before another document commits | A |
| B under A opens an `about:blank` popup whose DOM origin is B. | A fetch from the popup | A |
| B under A opens an `about:blank` popup, which embeds C. | A fetch from C | A, not B or C |
| B under A opens an `about:blank` popup, which embeds C. A `$csp` rule matches C. | The CSP added to C's response | A, not B or C |
| B under A opens an `about:blank` popup, which loads a child frame at C. A `$subdocument` rule matches C. | The navigation request for C | A |
| B under A opens a popup directly at C. | The navigation request for C | C |
| B under A opens a popup directly at C. | A fetch from C after commit | C |
| A opens a popup at B, which redirects to C, and C redirects back to B. | Each server records the request for its hop. | The destination of each hop: B, then C, then B |
| A opens a popup at B, which redirects to C. C responds with `Content-Security-Policy: sandbox`. | A fetch from the sandboxed C document | C, not A or B |
| B under A opens a popup at a URL that redirects to `javascript:`. Nothing commits. | A fetch from a child frame of the popup's initial empty document | A |
| An inherited popup with key A navigates to C. | A fetch from C after commit | C, not A or B |
| A sandboxed frame under A opens a popup. | A fetch from a child frame of the popup | A |
| Repeat the previous case with `noopener`. | A fetch from a child frame of the popup | A |
| An opaque inherited popup P opens an inheriting popup Q. | A fetch from Q | A |
| A popup is opened from a page whose key is defaults. | A fetch from the popup | Defaults |
| The browser navigates a popup from a site page to `data:`. | A fetch from the popup | Defaults, not the opener's key |
| A non-opaque inherited popup navigates itself to `about:blank`. | A fetch from the new document | A |

### Popup lifetime

In each case, P inherits A and has finished loading before the change.

| Change before the request | Measured request | Expected key |
| --- | --- | --- |
| P was opened by a sandboxed frame under A. The opener navigates from A to D. | A fetch from a child frame of P | A, not D |
| The opener commits a different site. | A fetch from P | A |
| The opener commits a different path on A. | A fetch from P | A |
| The opener changes its fragment. | A fetch from P | A |
| The opener calls `history.pushState`. | A fetch from P | A |
| The opener closes, and its `WebContents` is destroyed. | P's first fetch after destruction | A |
| The opening frame B navigates. | A fetch from P | A |
| The opening frame B is removed. | A fetch from P | A |
| P opens Q, then P closes. | A fetch from Q | A |
| P opens Q, then P navigates away. | A fetch from Q | A |
| P changes its own fragment. | A fetch from P | A |
| P starts a navigation that is canceled before commit. | A fetch from P's still-committed document | A |
| P navigates to C. | The navigation request for C | C |

### History and session restore

| Setup | Measured request | Expected key |
| --- | --- | --- |
| A sandboxed HTTP document on A navigates away. Back activates the same document from BFCache. | A fetch from the restored document | A |
| An inherited `about:blank?history` document in P is destroyed. The opener navigates to D, and P goes Back to that entry. | A fetch from the recreated document | A |
| The same as the previous case, but the opener is destroyed before Back. Keep BFCache enabled. | A fetch from the recreated document | A |
| The same history case, but P inherited defaults. | A fetch from the recreated document | Defaults |
| P inherits A and commits `about:blank?restore`. Close the opener, shut down the browser, and restore the last session. | A fetch from the restored P | A's current settings |
| Repeat the session restore case with a P that inherited defaults. | A fetch from the restored P | Current defaults |

### Workers and WebSockets

In each case, B is embedded in a top frame.

| Request source | Measured request | Expected key |
| --- | --- | --- |
| B under A | A WebSocket handshake | A |
| A dedicated worker of B under A | A fetch | A |
| A dedicated worker of B under A | A WebSocket handshake | A |
| A shared worker of B under A | A fetch | A |
| A shared worker of B under A | A WebSocket handshake | A |
| A service worker of B under A | A fetch | A |
| A service worker of B under A | A WebSocket handshake | A |
| A shared worker of B under a `data:` top frame | A fetch | Defaults, not B |
| A shared worker of B under a `data:` top frame | A WebSocket handshake | Defaults, not B |

## Required unit tests

Each row supplies inputs that a browser test cannot reliably create. Unit tests
must also keep the settings key separate from the origins used by adblock
rules.

| Inputs | Expected result |
| --- | --- |
| A frame has no stored key and no Brave observer. | Its committed non-opaque origin |
| A child request has no isolation info. | Rules use the outermost top-frame origin. Settings use the page's stored key if there is one. |
| A `kSubFrame` or `kOther` request has isolation info that conflicts with the frame's stored key. | The stored key, including stored defaults |
| A `kMainFrame` request has a non-opaque top-frame isolation origin and an outgoing frame token. | The isolation origin |
| A `kMainFrame` request has an opaque top-frame isolation origin. The outgoing document has an inherited site key or no key. | Defaults |
| A request with `kMainFrame` resource type has no isolation info. | The requesting document's key, or defaults without a frame |
| A worker request omits request context. | The worker factory's context |
| A worker request supplies request context. | The request context, not the factory's |
| A document-subresource request omits request context. | No factory fallback |
| A frameless WebSocket has an initiator but no usable isolation origin, or an opaque top-frame isolation origin. | Defaults |
| A worker request omits its initiator. | The factory's initiator, for rules only |
| A request has an original initiator and a different request initiator. | The original initiator, for rules only |
| The top-frame origin is opaque, with or without a precursor. | The raw opaque origin, unchanged |
| Rule initiators are missing, non-opaque, or opaque, for main-frame, subframe, and subresource requests. | The existing rule origins, unchanged |
| A request context is rebuilt from a supplied snapshot. | The supplied snapshot |
| A trusted main-frame redirect changes the key, keeps it, changes defaults to a site, or changes a site to defaults. | The redirect destination's key |
| A subframe or subresource redirect starts with a site key or defaults. | The captured key |
