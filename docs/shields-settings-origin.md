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

**Inheritance copies a value, not a reference to the opener.** Navigating or
closing the opener does not change the popup's settings. A same-document
navigation (`NavigationHandle::IsSameDocument()`), such as a fragment change,
preserves the value. Back/Forward to a retained document preserves its value;
if traversal recreates the document, restore the value saved on that
`NavigationEntry`. Restoring an `about:blank` entry must work after its original
`RenderFrameHost` has been destroyed and the opener has navigated elsewhere.

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
- **Unresolved handoff:** the implementation must identify the object holding
  B's settings for the navigation request, the method that populates it, and
  how it reaches the request snapshot before the first lookup. Attaching a value
  to B's document at `ReadyToCommitNavigation` is too late for that request.
- Do not change the adblock engine, `request_initiator_for_adblock_rules()`, or
  `first_party_origin_for_adblock_rules()`. Do not add a universal observer,
  renderer plumbing, or keepalive throttles.

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
- **History:** Verify destruction of the original document, navigate the opener
  elsewhere, and observe filtering after Back restores the same entry.
- **Unit tests:** Keep input/output tests for selection and precedence, including
  missing or opaque isolation origins. Use browser tests when the claim is that
  Chromium generates a particular request or delivers a callback at a particular
  time; do not supply that input or invoke that callback from the test.
