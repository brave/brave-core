# Peek security model

Peek previews a link in a card over the current tab (Shift+click, or "Open link
in Peek" in the link context menu). It is a new way of browsing, so this
document records how it handles each case the security team asks new browsing
modes to cover. Code lives in `//brave/browser/ui/peek` (platform-neutral logic,
navigation throttle) and `//brave/browser/ui/views/peek` (UI and the Peek
`WebContents` delegate). Tests are in
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

The general policy, agreed with the security team, is: **anything that needs the
user's trust decision moves Peek into a normal tab first** (the same
`WebContents`, so state is kept), and the request then continues there with the
tab's full browser UI. Anything that can't be handled safely is blocked. This
matches the iOS Quick Views review.

## Behavior by case

### Looking like browser UI

- The card hangs directly from the toolbar (its top edge is flush with the
  bottom of the toolbar, with no rounded top corners or gap) and its own toolbar
  uses the browser toolbar color. A page can't draw over the browser toolbar, so
  it can't fake that attachment.
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
its own content. The toolbar attachment and the omnibox staying visible are what
make the real one distinguishable.

### Security indicators (HTTP, HTTPS errors)

- The site info button uses `chrome_security_state::GetSecurityLevel` and the
  location bar's icon set, so HTTP pages show the "not secure" state and HTTPS
  pages the secure state.
- Certificate errors (for example badssl.com) show the normal SSL interstitial
  inside Peek, with the dangerous security state. "Open as tab" keeps the
  interstitial; the user's proceed or go-back decision then happens in a tab.
- Page Info opens for the Peek contents, not the underlying tab.

Tests: `HttpPageIsNotMarkedSecure`, `HttpsPageIsMarkedSecure`,
`CertErrorShowsInterstitial`, `SiteInfoButtonShowsPageInfoForPeek`.

### Permission prompts (permission.site)

- Peek tells the `PermissionRequestManager` that its tab is hidden, so no
  permission bubble can be shown from Peek. When a request comes in, the manager
  fails to show the prompt, Peek moves to a tab, the tab is marked active, and
  the prompt then shows anchored to that tab's omnibox.
- Camera and microphone requests go through the `WebContentsDelegate`; Peek
  moves to a tab and forwards the request to the tab's delegate.
  `CheckMediaAccessPermission` returns false while in Peek.
- Brave Wallet: requests that would open the wallet panel or account creation
  move Peek to a tab first (`brave_wallet_provider_delegate_impl_helper.cc`).

Tests: `PermissionRequestMovesToTab`.

### Downloads

- If the peeked link itself is a download, Peek closes (the download proceeds as
  if the link had been clicked in the tab) and no blank tab is left behind.
- If a page loaded in Peek starts a download, Peek moves to a tab so the
  download is attributed to a visible page.

Tests: `PeekedDownloadLinkClosesPeek`, `DownloadFromPeekedPageMovesToTab`.

### Non-http(s) navigations

- `peek::CanOpenInPeek` requires the target URL to be http(s) and the initiator
  to be a non-opaque http(s) origin, so Peek can't be opened from `chrome://`,
  `file:`, extension or opaque-origin pages, or to non-web URLs. The context
  menu item isn't offered in those cases.
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

- Popups without a user gesture are blocked. Chromium's popup blocker is a tab
  feature that Peek pages don't have, so `PeekContents::AddNewContents` refuses
  them itself, which closes the new window.
- Popups with a user gesture move Peek to a tab first, because the new window
  can script its opener; the popup then opens from that tab as usual.
- Fullscreen, pointer lock and keyboard lock requests move Peek to a tab and are
  forwarded to the tab's delegate, so the normal fullscreen bubble and exit UI
  are used.

Tests: `PopupWithoutGestureIsBlocked`, `PopupWithGestureMovesToTab`,
`FullscreenMovesToTab`.

### Dialogs

- Tab-modal dialogs (HTTP auth, print, and anything else using
  `WebContentsModalDialogManager`) move Peek to a tab before the dialog is
  shown.
- JavaScript dialogs (`alert`, `confirm`, `prompt`, `beforeunload`) are
  suppressed in Peek, because the Peek delegate has no
  `JavaScriptDialogManager`.
- File choosers (`<input type=file>`, including in cross-origin frames) move
  Peek to a tab before the chooser opens, since choosing which files to give the
  site is a trust decision. If there is no tab delegate after the move, the
  request is cancelled. Directory enumeration only follows a chooser, so it
  already runs in the tab; the Peek delegate's default cancels it.

Tests: `HttpAuthMovesToTab`, `FileChooserMovesToTab`,
`FileChooserInCrossSiteFrameMovesToTab`.

### Initiator origin and SameSite cookies

Both entry points pass the opener's origin, as required for new ways of opening
links (see brave/brave-core#39176), and the opener's `InitiatorNavigationState`.
The state is content/'s record of the initiating document, including its
policies. Without it, `NavigationRequest` builds no client security state for
the load, even with a correct origin, which is the shape of the failure in
Chromium's Link Preview (crbug.com/497336872). Chromium also documents the state
as required for renderer-initiated navigations and plans to enforce it
(crbug.com/510258191).

Local Network Access doesn't gate these loads today. `ResolveLocalNetworkAccess`
allows primary main frame navigations by default and only applies the permission
to subresources and subframe navigations, and a Peek load is a primary main
frame navigation, the same as a link opened in a tab. So the state keeps Peek
correct for what Chromium builds on it, rather than changing a visible LNA
outcome.

- Shift+click uses the `OpenURLParams` from `RenderFrameHostImpl::OpenURL`. The
  browser looks the state up from the renderer's tokens and terminates renderers
  that send missing tokens or tokens from another process, so a compromised
  renderer can't send an origin without the state.
- The context menu builds the params in the browser from the frame the menu was
  opened on: origin, frame token, process, site instance and
  `RenderFrameHost::GetCurrentInitiatorNavigationState()`, as a
  renderer-initiated link click. This follows the Lens and side panel link
  handlers. Nothing is opened if the frame is gone, no longer active or now has
  a different origin.
- `peek::CanOpenInPeek` rejects params without the state, so a new entry point
  that forgets it fails closed.

`PeekControllerImpl::Open` loads with `LoadURLParams(OpenURLParams)`, which
keeps the initiator, its state and the referrer. Cross-site Peek loads therefore
get `Sec-Fetch-Site: cross-site` and don't send `SameSite=Strict` cookies,
exactly like a link click. "Open as tab" moves the same `WebContents`, so
nothing is reloaded.

Tests: `ShiftClickSendsInitiatorOrigin`, `ContextMenuSendsInitiatorOrigin`,
`ContextMenuLoadCarriesInitiatorNavigationState` (the context-menu load has the
state, is renderer-initiated and has the source origin as initiator),
`OpenRequiresHttpInitiatorAndTarget`, and the `peek_utils` unit tests.

### Contexts: containers, sidebar, split view, private and Tor windows

- Containers: the Peek `WebContents` is created in the source tab's storage
  partition (`containers::MaybeInheritStoragePartition`), like a link opened in
  a new tab. Test: `InheritsContainerPartition`.
- Sidebar: Peek only opens from tabs in the same window. Sidebar web panels are
  excluded explicitly, since they are backed by tabs.
- Split view: Peek covers the whole contents area (both panes). "Open as tab"
  inserts the tab after the split, not inside it. Switching panes changes the
  active tab, which closes Peek. Test: `OpensFromSplitViewAsStandaloneTab`.
- Nested Peek: a Peek can't be opened from inside Peek; links there open in the
  Peek itself or, for new-tab dispositions, through the browser. Test:
  `NoNestedPeek`.
- Other windows: a window's Peek can only be opened from its own tabs. Test:
  `CannotOpenFromAnotherWindow`.
- Private and Tor windows: Peek uses the window's profile, so it is off the
  record (and proxied through Tor in Tor windows). Peek adds no prefs or on-disk
  state of its own. Test: `IncognitoPeekStaysOffTheRecord`.
- History: pages loaded in Peek are recorded like any other visit, so they're
  cleared with history. Peek attaches the history tab helper itself, except
  in private and Tor windows.

### Shields

- Shields protection applies in Peek like in a tab. Blocking runs in the
  network and renderer, keyed by the site's settings, and
  `BraveShieldsWebContentsObserver` is attached to every `WebContents`.
- The Shields button shows whether Shields is on for the site. It doesn't show
  a blocked count, which is tracked by the per-tab Shields helper.
- Pressing it moves the page into a tab and opens that tab's Shields panel, the
  same one the toolbar Shields button opens. Turning Shields off or allowing
  scripts is a decision about the site, so it's made with the tab's full
  browser UI, like permission prompts.

Test: `ShieldsButtonMovesToTabAndShowsPanel`.

## Security audit checklist

Results of going through the internal pull request security audit checklist for
the Peek change. Sections that don't apply (servers, OAuth, smart contracts,
dependencies, cryptography, and similar) are omitted.

- Test coverage: browser tests for every case above, plus unit tests for the URL
  and initiator checks.
- Initiator origin for new ways of opening links: done, see above.
- Origin determination: initiators come from
  `RenderFrameHost::GetLastCommittedOrigin` of the frame the context menu was
  opened on, or the browser-validated `OpenURLParams.initiator_origin`, never
  derived from a URL. Both carry the initiator's navigation state from content/,
  not only the origin.
- No remote content in the browser or WebUI process: previewed pages render in a
  normal renderer, like a tab.
- Private and Tor mode: no state written to disk, no leak into the regular
  profile, Tor windows' Peek uses the Tor profile and its network context.
- No new Mojo interfaces, renderer-exposed APIs, schemes, `window.*` injections,
  prefs, network requests or hostnames.
- Same-site vs same-origin: Peek's checks are scheme-based (http(s)); site
  permissions keep working per origin since they move to the tab.
- Clipboard: Peek doesn't write to the clipboard.

## Other browser UI

These go through code paths Peek doesn't intercept explicitly. Each was traced
to either move Peek to a tab first or fail closed while the page is in Peek:

- Device choosers (WebUSB, WebHID, WebSerial, Web Bluetooth):
  `chrome::ShowDeviceChooserDialog` only shows the chooser if the requesting
  contents are the active tab of a browser (`FindBrowserWithTab`). Peek's
  contents aren't a tab, so the chooser isn't shown and the request ends without
  a device. Open the page as a tab to use them.
- WebAuthn and passkey dialogs, Payment Request and Secure Payment Confirmation:
  shown with `constrained_window::ShowWebModalDialogViews`, i.e. as tab-modal
  dialogs, so Peek moves to a tab first, as for HTTP auth.
- FedCM: `IdentityDialogController` needs a `TabInterface` for the account
  chooser, so FedCM requests fail in Peek. The IdP registration prompt is a
  permission request and moves Peek to a tab.
- Password manager bubbles (save, update, manage): only shown for contents that
  are a tab in a browser (`ManagePasswordsUIController`), so they don't show
  from Peek.
- Autofill and password suggestions: the dropdown is shown in Peek, anchored to
  the field inside the card. This is the page's own UI rather than a prompt
  attached to the browser window: the card toolbar shows the site it belongs to,
  and the suggestions are the ones saved for that site.

- WebAuthn requests that go straight to the platform authenticator (iCloud
  Keychain on macOS, Windows Hello) without Chrome's dialog: the system dialog
  is attached to the browser window and can show while the page is in Peek.
  Accepted, like autofill: the system dialog names the site the passkey is for
  (the relying party ID, which must match the requesting site), and the passkey
  only works for that site. Moving Peek to a tab here would need a hook that
  can't tell these requests from conditional (autofill) passkey requests,
  which pages start as soon as they load.
