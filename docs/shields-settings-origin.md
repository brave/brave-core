# Shields settings origin

## Request lookup contract

`BraveRequestInfo::shields_settings_origin_` stores a `ShieldsSettingsOrigin`
for one request. Before `BraveRequestInfo::MakeCTX` calls
`IsBraveShieldsEnabled`, `GetAdControlType`, or `GetCosmeticFilteringControlType`,
`ctx->tab_origin()` must return `ctx->shields_settings_origin_.GetURL()`.
For site-specific settings this is the origin URL (scheme, host, effective
port); `GURL()` selects profile defaults. Defaults is a result, not an
instruction to try another origin. Never use an opaque origin's precursor.

### NAV-1: two requests in one tab before a cross-site commit

1. Commit `https://a.test/old` in the primary main frame. Execute
   `location.assign("https://b.test/new")` in that document. Assume no redirect,
   cancellation, or competing navigation.
2. In `WebContentsObserver::DidStartNavigation(handle)`,
   `NavigationRequest::StartNavigation()` has set `WILL_START_REQUEST`.
   `handle->IsInPrimaryMainFrame()` is true, `handle->IsSameDocument()` and
   `handle->HasCommitted()` are false, and `handle->GetURL()` is
   `https://b.test/new`. At this callback,
   `web_contents->GetPrimaryMainFrame()->GetLastCommittedURL()` is
   `https://a.test/old`.
3. For the initial main-frame HTTP request for `/new`,
   `ctx->shields_settings_origin_.GetURL()` must equal `GURL("https://b.test/")`
   **before the first settings lookup**, before `ReadyToCommitNavigation` for B.
   Configuring A to `ALLOW` and B to `BLOCK` must yield `BLOCK` from
   `GetAdControlType` for this request; reversing them must yield `ALLOW`.
4. For a subresource request made by the document at `https://a.test/old`
   before B commits, the field on **that request's** `BraveRequestInfo` must
   equal `GURL("https://a.test/")` at lookup. Starting the navigation to B
   must not overwrite A's document value.
5. In `DidFinishNavigation(handle)`, a successful, non-error commit to B has
   `handle->HasCommitted()` true and `handle->IsErrorPage()` false. Subsequent
   requests from the committed B document use B. If B was canceled without a
   commit, A's stored document value remains A.

For a primary-main-frame network request with no usable site origin, the
field must instead contain a value whose `GetURL()` equals `GURL()`, even if
its frame token resolves to A's `RenderFrameHost`.

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
`shields_settings_origin_` for NAV-1 step 3, and which method assigns the field
before the first settings lookup? The branch currently assigns it in `MakeCTX`
by calling `ctx->GetShieldsSettingsOrigin(isolation_info)`. That getter computes
the value; it does not satisfy this contract.

## Acceptance evidence

- For NAV-1, observe what the destination server receives for `/new` with both
  configurations of A and B. A subresource fetch after B commits is insufficient.
- For the initial empty popup, observe a request issued by that document before
  a subsequent main-frame document commits. A later popup document or a child
  frame cannot serve as its substitute.
- Use separate browser tests for distinct Chromium lifecycle scenarios, opposing
  site settings, and a successful unfiltered request as a control. Unit tests
  verify outputs for supplied inputs; supplying isolation info, a frame token,
  or an observer callback is not evidence of when Chromium supplies it.
