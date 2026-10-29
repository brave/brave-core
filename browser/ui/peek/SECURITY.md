# Peek security model

Peek previews a link in a card over the current tab (Shift+click, or "Open
link in Peek" in the link context menu). It is a new way of browsing, so this
document records how it handles each case the security team asks new browsing
modes to cover. Code lives in `//brave/browser/ui/peek` (platform-neutral
logic, navigation throttle) and `//brave/browser/ui/views/peek` (UI and the
Peek `WebContents` delegate). Tests are in
`//brave/browser/ui/views/peek/peek_browsertest.cc` and
`//brave/browser/ui/peek/peek_utils_unittest.cc`.

Peek is behind `brave://flags/#brave-peek` and is desktop-only.

## Threat model

The previewed page is untrusted. Peek must not let it:

1. Pass page content off as browser UI, or browser UI off as page content
   (origin spoofing).
2. Get capabilities it would not get in a normal tab, or get them through a
   prompt the user can't tie to the right page.
3. Reach schemes or contexts that a link click from a web page could not.
4. Escape the storage, profile, or network context of the tab it was opened
   from.

The general policy, agreed with the security team, is: **anything that needs
the user's trust decision moves Peek into a normal tab first** (the same
`WebContents`, so state is kept), and the request then continues there with
the tab's full browser UI. Anything that can't be handled safely is blocked.
This matches the iOS Quick Views review.

## Behavior by case

### Looking like browser UI

- The card hangs directly from the toolbar (its top edge is flush with the
  bottom of the toolbar, with no rounded top corners or gap) and its own
  toolbar uses the browser toolbar color. A page can't draw over the browser
  toolbar, so it can't fake that attachment.
- The card toolbar has the site info (security) button, the Shields button,
  "Open as tab" and close.
- The host shown in the card toolbar is
  `url_formatter::FormatUrlForSecurityDisplay(GetVisibleURL())`, the same
  formatting used for security-sensitive URL display elsewhere. The page title
  is shown next to it and is page-controlled; the host is the authoritative
  part.
- Only the content area below the toolbar is dimmed; the real omnibox stays
  visible and keeps showing the tab the user is on.

This is a mitigation, not a full fix: a page can still draw a fake card inside
its own content. The toolbar attachment and the omnibox staying visible are
what make the real one distinguishable.

### Security indicators (HTTP, HTTPS errors)

- The site info button uses `chrome_security_state::GetSecurityLevel` and the
  location bar's icon set, so HTTP pages show the "not secure" state and
  HTTPS pages the secure state.
- Certificate errors (for example badssl.com) show the normal SSL interstitial
  inside Peek, with the dangerous security state. "Open as tab" keeps the
  interstitial; the user's proceed or go-back decision then happens in a tab.
- Page Info opens for the Peek contents, not the underlying tab.

Tests: `HttpPageIsNotMarkedSecure`, `HttpsPageIsMarkedSecure`,
`CertErrorShowsInterstitial`, `SiteInfoButtonShowsPageInfoForPeek`.

### Permission prompts (permission.site)

- Peek tells the `PermissionRequestManager` that its tab is hidden, so no
  permission bubble can be shown from Peek. When a request comes in, the
  manager fails to show the prompt, Peek moves to a tab, the tab is marked
  active, and the prompt then shows anchored to that tab's omnibox.
- Camera and microphone requests go through the `WebContentsDelegate`; Peek
  moves to a tab and forwards the request to the tab's delegate.
  `CheckMediaAccessPermission` returns false while in Peek.
- Brave Wallet: requests that would open the wallet panel or account creation
  move Peek to a tab first (`brave_wallet_provider_delegate_impl_helper.cc`).

Tests: `PermissionRequestMovesToTab`.

### Downloads

- If the peeked link itself is a download, Peek closes (the download proceeds
  as if the link had been clicked in the tab) and no blank tab is left behind.
- If a page loaded in Peek starts a download, Peek moves to a tab so the
  download is attributed to a visible page.

Tests: `PeekedDownloadLinkClosesPeek`, `DownloadFromPeekedPageMovesToTab`.

### Non-http(s) navigations

- `peek::CanOpenInPeek` requires the target URL to be http(s) and the
  initiator to be a non-opaque http(s) origin, so Peek can't be opened from
  `chrome://`, `file:`, extension or opaque-origin pages, or to non-web URLs.
  The context menu item isn't offered in those cases.
- `PeekNavigationThrottle` runs on every main-frame navigation in a Peek
  `WebContents` and cancels anything that isn't http(s), at request start, on
  each redirect, and for navigations that commit without a URL loader
  (`about:blank`). This covers `data:`, `blob:`, `chrome://`, `file:` and
  external protocols such as `mailto:`. Subframes are unaffected.
- After "Open as tab" the throttle no longer applies; the tab behaves like any
  other tab.

Tests: `BlocksNonHttpNavigations`, `BlocksRedirectToNonHttp`,
`OpenRequiresHttpInitiatorAndTarget`, `NoPeekMenuItemOnBrowserPages`, and the
`peek_utils` unit tests.

### Popups and fullscreen

- Popups without a user gesture are blocked.
- Popups with a user gesture move Peek to a tab first, because the new window
  can script its opener; the popup then opens from that tab as usual.
- Fullscreen, pointer lock and keyboard lock requests move Peek to a tab and
  are forwarded to the tab's delegate, so the normal fullscreen bubble and
  exit UI are used.

Tests: `PopupWithoutGestureIsBlocked`, `PopupWithGestureMovesToTab`,
`FullscreenMovesToTab`.

### Dialogs

- Tab-modal dialogs (HTTP auth, print, and anything else using
  `WebContentsModalDialogManager`) move Peek to a tab before the dialog is
  shown.
- JavaScript dialogs (`alert`, `confirm`, `prompt`, `beforeunload`) are
  suppressed in Peek, because the Peek delegate has no
  `JavaScriptDialogManager`.
- File choosers are forwarded to the browser and attach to the browser window.

Tests: `HttpAuthMovesToTab`.

### Initiator origin and SameSite cookies

Both entry points pass the opener's origin, as required for new ways of
opening links (see brave/brave-core#39176):

- Shift+click uses the renderer's `OpenURLParams`, including
  `initiator_origin` and referrer.
- The context menu uses `GetOpenURLParamsWithExtraHeaders` with the frame's
  committed origin, like "Open link in new tab".

`PeekControllerImpl::Open` loads with `LoadURLParams(OpenURLParams)`, which
keeps the initiator and referrer. Cross-site Peek loads therefore get
`Sec-Fetch-Site: cross-site` and don't send `SameSite=Strict` cookies, exactly
like a link click. "Open as tab" moves the same `WebContents`, so nothing is
reloaded.

Tests: `ShiftClickSendsInitiatorOrigin`, `ContextMenuSendsInitiatorOrigin`.

### Contexts: containers, sidebar, split view, private and Tor windows

- Containers: the Peek `WebContents` is created in the source tab's storage
  partition (`containers::MaybeInheritStoragePartition`), like a link opened
  in a new tab. Test: `InheritsContainerPartition`.
- Sidebar: Peek only opens from tabs in the same window. Sidebar web panels
  are excluded explicitly, since they are backed by tabs.
- Split view: Peek covers the whole contents area (both panes). "Open as tab"
  inserts the tab after the split, not inside it. Switching panes changes the
  active tab, which closes Peek. Test: `OpensFromSplitViewAsStandaloneTab`.
- Nested Peek: a Peek can't be opened from inside Peek; links there open in
  the Peek itself or, for new-tab dispositions, through the browser. Test:
  `NoNestedPeek`.
- Other windows: a window's Peek can only be opened from its own tabs. Test:
  `CannotOpenFromAnotherWindow`.
- Private and Tor windows: Peek uses the window's profile, so it is off the
  record (and proxied through Tor in Tor windows). Peek adds no prefs or
  on-disk state of its own. Test: `IncognitoPeekStaysOffTheRecord`.
- History: pages loaded in Peek are recorded like any other visit (the normal
  tab helpers are attached), so they're cleared with history.

## Security audit checklist

Results of going through the internal pull request security audit checklist
for the Peek change. Sections that don't apply (servers, OAuth, smart
contracts, dependencies, cryptography, and similar) are omitted.

- Test coverage: browser tests for every case above, plus unit tests for the
  URL and initiator checks.
- Initiator origin for new ways of opening links: done, see above.
- Origin determination: initiators come from
  `RenderFrameHost::GetLastCommittedOrigin` (context menu `frame_origin`) or
  the browser-validated `OpenURLParams.initiator_origin`, never derived from a
  URL.
- No remote content in the browser or WebUI process: previewed pages render in
  a normal renderer, like a tab.
- Private and Tor mode: no state written to disk, no leak into the regular
  profile, Tor windows' Peek uses the Tor profile and its network context.
- No new Mojo interfaces, renderer-exposed APIs, schemes, `window.*`
  injections, prefs, network requests or hostnames.
- Same-site vs same-origin: Peek's checks are scheme-based (http(s)); site
  permissions keep working per origin since they move to the tab.
- Clipboard: Peek doesn't write to the clipboard.

## Not yet verified

These go through code paths Peek doesn't intercept explicitly. They're expected
to either fail closed (no dialog host) or show browser-owned UI, but haven't
been tested in Peek:

- Device choosers (WebUSB, WebHID, WebSerial, Web Bluetooth).
- WebAuthn and passkey prompts.
- Payment Request.
- FedCM.
- Autofill and password manager bubbles.
