# Peek

Spec for Peek, a link preview for Brave desktop. Written against the
[spec checklist](https://github.com/brave/handbook/blob/master/specs/checklist.md).
The security model and the per-case security behavior are in
[SECURITY.md](SECURITY.md); this document links to it instead of repeating it.

## Status

`In Development`. Behind `brave://flags/#brave-peek`, disabled by default.

## Team

| Role | Who |
| --- | --- |
| Developer (Desktop) | Agustín Ruiz |
| QA | Not assigned yet |
| Design | Not assigned yet |
| PM | Not assigned yet |
| Security | Reviewing in the PR |

## Objective

Let a user look at a link without leaving the page they are on. Peek opens the
link in a card over the current tab. The user can read it, check it, and then
dismiss it, or keep it by moving it into a tab. The page behind stays exactly as
it was.

Triggers:

- Shift+click on a link. This replaces the browser's default "open in new
  window" for Shift+click.
- "Open Link in Peek" in the link context menu.

## Relation to company objectives

Peek is a browsing-flow feature. It makes following links cheaper, so users keep
their place in the page they are reading and don't open tabs they will close
right away. It adds no accounts, services, or data collection.

## Non-goals

- Android and iOS. iOS has its own link preview (Quick Views), reviewed
  separately.
- Previewing anything other than http(s) pages.
- Opening Peek from non-web pages (`chrome://`, `file:`, extension pages).
- More than one Peek per window.
- Peek inside a Peek.
- Letting Peek pages ask for permissions, open the wallet, go fullscreen, or
  open windows without leaving Peek. Those move the page into a tab.
- Growing the card from the position of the clicked link. Possible later.
- Triggers other than Shift+click and the context menu.
- Keeping the Shields panel loaded between Peeks.

## Target platforms

Desktop: macOS, Windows, Linux. Android and iOS are not supported. The
feature flag is only listed on desktop (`kOsDesktop`) and the code is under
`TOOLKIT_VIEWS`.

## Channels and feature flag

`peek::features::kBravePeek`, `FEATURE_DISABLED_BY_DEFAULT`, listed as
`brave-peek` ("Brave Peek") in `brave://flags`. It lands on all channels with
the flag off, so it can land in pieces without reaching users. With the flag
off, no controller is created, the context menu item is not added, and
Shift+click behaves as it did.

## Changes from the current behavior

| Today | With Peek |
| --- | --- |
| Shift+click on a link opens it in a new window | Opens it in Peek (flag on) when the source is a tab in this window and the link is http(s) |
| Link context menu has no preview item | Adds "Open Link in Peek" for http(s) links on http(s) pages |
| A link can only be previewed by opening it in a tab | Peek loads it without adding a tab or changing the active tab |
| The page info bubble shows the combined Shields view for any contents | Contents that are not tabs, such as Peek, get the plain page info bubble |
| A Shields panel always controls the active tab | `ShieldsPanelUI::SetTargetWebContents()` lets it control another `WebContents` |

## Behavior

### States

| State | What the user sees | Leaves the state when |
| --- | --- | --- |
| Closed | Nothing | A Peek is opened |
| Opening | Card grows from 90% to full size and the dim backdrop fades in over 200 ms. Skipped when reduced motion is on | The animation ends, or the user acts |
| Loading | Card with toolbar. The page loads inside it | The page commits or fails |
| Open | The loaded page. Toolbar shows security state, favicon, title, host, Shields count, "Open as tab", close | Any of the exits below |
| Open, error | The browser's error page or certificate interstitial, inside the card | The user leaves, or moves it to a tab |
| Moving to tab | The page moves to a new tab next to the current one and the card goes away | Done |
| Closed by user | The card and its page are discarded | Done |

Exits from Open:

- Escape, the close button, a click outside the card, or `window.close()` from
  the page. All close Peek and discard the page.
- "Open as tab". Moves the loaded page into a new active tab next to the current
  one. No reload.
- Switching tabs. Closes Peek.
- Opening another Peek. Replaces the current one.
- The page asks for something that needs a normal tab (see
  [SECURITY.md](SECURITY.md)). Peek moves to a tab first, then the request
  continues there.

### Persistent and non-persistent state

Nothing in Peek is persisted by Peek itself. There are no preferences, no
files, and no new per-profile data.

- A Peek page is a normal page load in the window's profile, or in the source
  tab's container. Its history entry, cookies, and site storage behave like any
  other page's.
- After "Open as tab" the page is an ordinary tab, including session restore.
- A Peek that is closed or replaced leaves nothing behind beyond the history
  entry and site data above.

### Clearing browsing data

Peek has no data of its own to clear. History, cookies, and site data from
pages loaded in Peek are cleared with the normal "Clear browsing data" actions,
because Peek pages use the normal tab helpers and storage.

### Window types

| Window | Behavior |
| --- | --- |
| Normal | As described |
| Private | Peek uses the private window's profile, so it is off the record |
| Tor | Peek uses the Tor window's profile. Not tested with a Tor window yet |
| Another window | A window's Peek can only be opened from tabs in that window |

Container tabs: the Peek page loads in the same container storage as the tab it
was opened from. Split view: the card covers the whole contents area, both
panes, and "Open as tab" adds a standalone tab after the split. Sidebar web
panels cannot open Peek.

### Errors and edge cases

| Condition | Behavior |
| --- | --- |
| Target is not http(s) | Not offered in the context menu. `Open()` refuses it |
| Source page is not http(s) (`chrome://`, `file:`, extension, opaque origin) | Not offered. `Open()` refuses it |
| Source is a Peek, a sidebar panel, or a tab from another window | Not offered |
| The Peek page navigates to a non-http(s) URL, or redirects to one | Navigation is cancelled. The page stays on its last URL |
| The page loads `about:blank`, `data:` or `blob:` at top level | Cancelled |
| Certificate error | The normal interstitial shows in the card. Security button shows the dangerous state. "Open as tab" keeps the interstitial |
| HTTP page | Security button shows not secure |
| Network error | The browser's error page shows in the card |
| Page requests a permission, camera/mic, wallet, fullscreen, pointer or keyboard lock | Peek moves to a tab, then the request continues |
| Page opens a popup with a user gesture | Peek moves to a tab, then the popup opens |
| Page opens a popup without a gesture | Blocked |
| HTTP auth or another tab-modal dialog | Peek moves to a tab, then the dialog shows |
| `alert`, `confirm`, `prompt`, `beforeunload` | Suppressed while in Peek |
| The peeked link is a download | Peek closes and the download proceeds |
| The page starts a download | Peek moves to a tab |
| File chooser | Forwarded to the browser and attached to the browser window |
| Peek is opened while another is open | The new one replaces it |
| The active tab changes | Peek closes |

Not specified yet: what Peek shows if its renderer crashes. See open questions.

## UI

### Layout

The card is a frameless child widget over the contents area of the browser
window. Views cannot paint above the native web view on every platform, so a
separate widget is used.

- Top: flush with the bottom of the browser toolbar. No gap and no rounded top
  corners, so it looks like it hangs from the toolbar. This makes it harder for
  a page to draw a convincing fake. It does not prevent a page from drawing a
  fake card inside its own content.
- Left, right, bottom: inset by the larger of 24 px or 7.5% of the covered width
  (left and right) or height (bottom).
- Bottom corners: the browser's contents radius
  (`kRoundedCornersBorderRadius`, 6 px), so the card matches the rounded page
  area around it. Top corners: 0, because the card is attached to the toolbar.
- Backdrop: only the contents area is dimmed. The browser toolbar and omnibox are
  not.
- Toolbar inside the card: 40 px high, 12 px horizontal padding, 8 px between
  items. The title and host shrink (and elide) when space runs out, so the site
  info, Shields, Open as tab and Close buttons always stay visible. Uses the browser toolbar color.
- Divider: 1 px, full card width, between the card toolbar and the page.
- Icons and favicon: 16 px.
- Shadow: offset y 4 px, blur 24 px, 24% opacity.
- Animation: 200 ms, ease out, from 90% size to full size. Backdrop fades in
  over the same time. The top edge stays attached to the toolbar while it grows.

### Toolbar contents, left to right

1. Site info button. Opens the standard page info bubble for the Peek page.
   Shows the tune icon on secure pages and the location bar security icon
   otherwise (HTTP, certificate errors).
2. Favicon.
3. Title, left aligned, tail elided.
4. Host, left aligned, head elided. Formatted with
   `FormatUrlForSecurityDisplay` and without the scheme for http(s).
5. Shields button. Opens the Shields panel for the Peek page, not the tab
   behind. Its tooltip reports the Shields state of the Peek page.
6. "Open as tab" button.
7. Close button.

### Colors

All Nala tokens, so light and dark themes follow the browser.

| Part | Token |
| --- | --- |
| Card background | `nala::kColorContainerBackground` |
| Card toolbar | `kColorToolbar` |
| Divider | `nala::kColorDividerSubtle` |
| Title | `nala::kColorTextPrimary` |
| Host | `nala::kColorTextTertiary` |
| Icons | `nala::kColorIconDefault`, `nala::kColorIconDisabled` |
| Backdrop | `nala::kColorDialogsScrimBackground`, scaled by animation progress |
| Shadow | `nala::kColorSchemesShadow` at 24% |

`nala::kColorDividerFaint` was too close to white in the light theme to be seen
against the toolbar and page, so the divider uses subtle.

### Responsive behavior

The card is sized from the contents area, so it follows window resizes and
split view. The 24 px minimum inset keeps a visible backdrop on small windows.
The toolbar keeps the buttons at a fixed size. When the card is narrow or the
title is long, the host elides first, then the title.

### Platform differences

None in layout or behavior. It is the same views code on macOS, Windows and
Linux.

### Accessibility

- The Peek view has the dialog role and the accessible name "Peek".
- Every toolbar button has a tooltip that is also its accessible name.
- Escape closes Peek.
- Opening Peek focuses the page inside it. Tab and Shift+Tab leaving the page
  move focus to the card's toolbar, and focus cycles inside the card.
- Reduced motion turns the opening animation off.
- Colors come from Nala tokens, which are designed for contrast in both themes.

Not checked yet: where focus returns to when Peek closes, and screen reader
announcements when it opens. The Tab-out-of-page test has not been run. See open questions.

### Strings

| ID | Text | Use |
| --- | --- | --- |
| `IDS_PEEK_ACCESSIBLE_NAME` | Peek | Accessible name of the card. "Peek" is the feature name |
| `IDS_PEEK_OPEN_AS_TAB` | Open as tab | Tooltip and accessible name of the button |
| `IDS_CONTENT_CONTEXT_OPENLINKPEEK` | Open Link in Peek (title case variant on macOS) | Context menu item |
| `IDS_APP_CLOSE` | Close | Existing string, reused for the close button |
| Flag name | Brave Peek | `brave://flags` |
| Flag description | See below | `brave://flags` |

The flag description in `about_flags.cc` still says Peek is triggered by
"cross-site links in pinned tabs", which Peek no longer does. The description
needs updating before this lands.

Strings need another pass once the feature is in a real build, and must be in by
the time it reaches the Dev channel.

### Mockups and screenshots

Screenshots are added to the PR. Nala tokens above stand in for color values.

## Technical design

- `//brave/browser/ui/peek:public` has `PeekController`, a per-window interface
  owned by `BraveBrowserWindowFeatures`, and the `PeekShieldsPanelProvider`
  interface.
- `//brave/browser/ui/peek` has the shared logic: URL and initiator checks
  (`peek_utils`) and `PeekNavigationThrottle`.
- `//brave/browser/ui/peek:context_menu` has two small hooks that the
  `chromium_src` override of `render_view_context_menu.cc` calls, so the override
  does not include Peek headers.
- `//brave/browser/ui/views/peek` has `PeekControllerImpl`, `PeekView`,
  `PeekShieldsButton`, and `PeekContents`. `PeekContents` owns the page and is its
  `WebContentsDelegate`, `WebContentsModalDialogManagerDelegate`,
  `PermissionRequestManager::Observer` and `DownloadManager::Observer`.
- Peek pages get the full tab helper set. A rewrite rule and patch for
  `tab_helpers.h` make `PeekContents` a friend of `TabHelpers`.
- Shift+click goes through `BraveBrowserWebContentsDelegate::OpenURLFromTab`.
  The context menu uses `IDC_CONTENT_CONTEXT_OPENLINKPEEK`.
- `PeekController::Open(source, params)` returns false unless the source is a
  tab in this window and the params pass `CanOpenInPeek` (http(s) target,
  non-opaque http(s) initiator).
- `BraveBrowserView` gives the controller a `ShieldsPanelPeekProvider`, which
  points the Shields panel at the Peek page.
- Shared code that changed: `ShieldsPanelUI::SetTargetWebContents()`,
  `BraveShieldsActionController` icon and tooltip helpers,
  `BravePageInfoBubbleView` for non-tab contents, and `ViewShadow` moved to its own target.
- `BraveContentBrowserClient` registers `PeekNavigationThrottle`, including for
  commits without a URL loader.
- Brave Wallet's provider delegate moves Peek into a tab before it shows a panel
  or account creation.

## Privacy and security

- Network requests: none from Peek itself. The previewed page loads like any
  tab, through the window's profile network context (Tor in Tor windows).
- Encryption and hashing: none.
- Persisted sensitive information: none added.
- Referrer and initiator: Peek loads carry the opener's origin and referrer, so
  SameSite cookies and Fetch Metadata match a link click.
- The security team has reason to review this: it is a new way to browse, with a
  web page rendered inside browser-owned UI. The review is in the PR. The model,
  per-case behavior and the audit checklist results are in
  [SECURITY.md](SECURITY.md).

## Devops

No service, download, CDN, proxy, or scheduled job is needed.

## P3A

None added. Whether to measure Peek usage is an open question.

## Testing and QA

- Unit tests: `peek_utils_unittest.cc` (URL and initiator eligibility, Shift+click
  rules).
- Browser tests: `peek_browsertest.cc`. They cover each trigger, dismissal,
  open as tab, the site info and Shields buttons, the card attachment, each
  security case in [SECURITY.md](SECURITY.md), containers, split view, nested
  Peek, other windows and incognito.
- A QA plan has not been written yet. Manual steps to start from:
  - Enable `brave://flags/#brave-peek`.
  - Shift+click a link and use "Open Link in Peek", in a normal tab.
  - Dismiss with Escape, the close button, a click outside, and switching tabs.
  - "Open as tab" keeps the page without reloading.
  - Site info and Shields act on the Peek page.
  - Open https://permission.site/ and https://badssl.com/ pages in Peek.
  - From a Peek page try a download, `window.open`, fullscreen and a link to a
    `data:` URL.
  - Peek a link from a container tab, a split view, a private window and a Tor
    window.
  - With the flag off, nothing changes.

## Phases

1. Peek with Shift+click, the context menu, the toolbar, Shields and open as tab.
   This is in the PR.
2. Security hardening: toolbar anchoring, navigation blocking, promotion to tab,
   initiator origin, containers. Also in the PR.
3. Later: a better opening animation (the card is resized each frame today,
   which resizes the web view), growing from the clicked link, keeping the
   Shields panel loaded across Peeks, more triggers.

No time estimate is given for phase 3.

## Related specs and conflicts

No conflicting in-progress spec is known. Peek touches Split View, Sidebar,
Containers, Shields and Brave Wallet, and the interactions are described above.

## Acronyms

- P3A: Privacy-Preserving Product Analytics.
- Nala: Brave's design system, with the color tokens used above.
- SameSite: the cookie attribute that limits cookies on cross-site requests.
- Fetch Metadata: the `Sec-Fetch-*` request headers.

## Open questions

1. Who are the QA, design and PM owners, and who is the co-maintainer for
   security reports?
2. Should Peek record any P3A metric, such as how often it opens and how often it
   becomes a tab?
3. What should Peek show if its renderer crashes?
4. Where should focus go when Peek closes, and should screen readers announce it
   when it opens?
5. The flag description still mentions pinned-tab links. Should it just list
   Shift+click and the context menu?
6. Should Peek be available in Tor windows, or only normal and private ones?
7. Device choosers (WebUSB, WebHID, WebSerial, Web Bluetooth), WebAuthn, Payment
   Request, FedCM, and autofill or password manager bubbles are not verified in
   Peek. Do they need explicit handling?
8. `PeekBrowserTest.EscapeClosesPeek` fails locally. Escape does not reach the
   Peek page's keyboard handler. The cause is not known yet.
