# Shields settings origin

## Request lookup contract

`BraveRequestInfo::shields_settings_origin_` stores a `ShieldsSettingsOrigin`
for one request. Before `BraveRequestInfo::MakeCTX` calls
`IsBraveShieldsEnabled`, `GetAdControlType`, or `GetCosmeticFilteringControlType`,
`ctx->tab_origin()` must return `ctx->shields_settings_origin_.GetURL()`.
For site-specific settings this is the origin URL (scheme, host, effective
port); `GURL()` selects profile defaults. Defaults is a result, not an
instruction to try another origin. Never use an opaque origin's precursor.

## Main-frame HTTP requests use the destination site's settings

When the primary main frame navigates from `https://a.test/old` to
`https://b.test/new` without a redirect, the HTTP request for `/new` uses B's
Shields settings. In `WebContentsObserver::DidStartNavigation(handle)`,
`NavigationRequest::StartNavigation()` has set `WILL_START_REQUEST`:
`handle->IsInPrimaryMainFrame()` is true, `handle->IsSameDocument()` and
`handle->HasCommitted()` are false, and `handle->GetURL()` is
`https://b.test/new`. At this callback,
`web_contents->GetPrimaryMainFrame()->GetLastCommittedURL()` is `https://a.test/old`.

For the request for `/new`, `ctx->shields_settings_origin_.GetURL()` must equal
`GURL("https://b.test/")` before the first settings lookup in `MakeCTX`, before
`ReadyToCommitNavigation` for B. Configuring A to `ALLOW` and B to `BLOCK`
must yield `BLOCK` from `GetAdControlType`; reversing them must yield `ALLOW`.
If a primary-main-frame network request has no usable site origin, the settings
URL must be `GURL()`, even if its frame token resolves to A's `RenderFrameHost`.

## Subresource requests use the committed top-level document's settings

If the document at `https://a.test/old` issues a fetch after navigation to B
starts but before B commits, `ctx->shields_settings_origin_.GetURL()` for the
fetch must equal `GURL("https://a.test/")`. Starting the navigation to B must
not overwrite A's document value.

In `DidFinishNavigation(handle)`, a successful, non-error commit to B has
`handle->HasCommitted()` true and `handle->IsErrorPage()` false. Fetches from
the committed B document use B. If the navigation is canceled without a commit,
fetches from A continue to use A.

## Document and factory cases

| Event | Required settings origin |
| --- | --- |
| B is a child of primary main frame A and calls `fetch` for a resource on C. | A. |
| B under A calls `window.open()`; the popup's initial empty main-frame document makes a request. | A. Navigating or closing the opener does not change the captured value. |
| B under A opens an inheriting `about:blank` popup; its committed document makes a request. | A. This does not establish behavior of the initial empty document. |
| That popup navigates to `https://c.test/page`. | C for the HTTP request for `/page` and for requests from the committed C document. |
| A commits with a sandboxed opaque DOM origin, then navigates itself to `about:blank`. | A for the sandboxed document, from its site navigation; the `about:blank` document inherits that stored value. |
| A sandboxed child B under A initiates an allowed top-frame navigation to `about:blank`. | A for the resulting document. |
| A browser-initiated `data:` navigation commits with no inherited settings value. | `GURL()` for requests from that document. |
| `DidFinishNavigation(handle)` reports `IsSameDocument()` for an inherited popup's fragment navigation. | Preserve the document's stored value. |
| Back recreates an inherited document after its `RenderFrameHost` was destroyed. | Restore the committed `NavigationEntry`'s saved value, independently of the opener. |
| A worker request has no frame token or request-level trusted isolation info. | Use the factory isolation info's usable top-frame site, or defaults if opaque or absent. |

A WebSocket request whose `resource_type` was left unset must not be treated
as a primary-main-frame navigation solely because the field's numeric default
denotes `kMainFrame`.

## Ownership and unresolved handoff

`ShieldsSettingsOrigin` owns selection. `BraveShieldsWebContentsObserver` owns
lifecycle writes for navigations, documents, popups, and history entries.
`GetShieldsSettingsOrigin` and `tab_origin()` are accessors: neither may
classify the request, resolve a frame token, inspect isolation info, or choose
between candidate origins. The adblock engine and its two inputs,
`request_initiator_for_adblock_rules()` and `first_party_origin_for_adblock_rules()`,
are outside this change.

**Open design question:** Which production object supplies
`shields_settings_origin_` for the main-frame HTTP request for `/new`, and which method assigns the field
before the first settings lookup? The branch currently assigns it in `MakeCTX`
by calling `ctx->GetShieldsSettingsOrigin(isolation_info)`. That getter computes
the value; it does not satisfy this contract.

## Acceptance evidence

- For the main-frame navigation, observe what the destination server receives for `/new` with both
  configurations of A and B. A subresource fetch after B commits is insufficient.
- For the initial empty popup, observe a request issued by that document before
  a subsequent main-frame document commits. A later popup document or a child
  frame cannot serve as its substitute.
- Use separate browser tests for distinct Chromium lifecycle scenarios, opposing
  site settings, and a successful unfiltered request as a control. Unit tests
  verify outputs for supplied inputs; supplying isolation info, a frame token,
  or an observer callback is not evidence of when Chromium supplies it.
