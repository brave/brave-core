# M156 code health follow-ups

Tasks found by reviewing upstream changes in `155.0.8059.30..156.0.8078.4` (8957
commits) for regressions, deprecations and code-health migrations that apply to
Brave. Tick items off as they land.

Items that span cr156 and cr157 are tracked in `M157_codehealth.md`.

## 1. Regressions

### Payments page overrides no longer apply

Upstream migrated `settings-payments-page` to Lit
([cfc98f3d8db5d](https://chromium.googlesource.com/chromium/src/+/cfc98f3d8db5d)).
`browser/resources/settings/br/payments_page.ts` still uses
`RegisterPolymerTemplateModifications`, which never runs for a Lit element, so
the hidden controls are visible again. This fails silently.

- [x] Port `br/payments_page.ts` to
      `chromium_src/chrome/browser/resources/settings/autofill_page/payments/payments_page.html.ts.lit_mangler.ts`
      and remove the Polymer override (as in the security page port):
  - [x] Hide `#manageLink` (declared in the element's `$`, so hide it rather
        than remove it).
  - [x] Hide `#cardBenefitsToggle`.

### Avatar button shows Google's "Sign in" pill again

Brave forces `IsReplaceSyncPromosWithSignInPromosEnabled()` to false
(`rewrite/components/sync/base/features.cc.yaml`). Upstream removed that gate
from `ComputeProfileMenuAvatarButtonPromoInfo()`
([40151b7daac01](https://chromium.googlesource.com/chromium/src/+/40151b7daac01),
[56f48e0db9815](https://chromium.googlesource.com/chromium/src/+/56f48e0db9815),
[65eb31026d38b](https://chromium.googlesource.com/chromium/src/+/65eb31026d38b)).
A signed-out profile now gets the sign-in promo whenever
`CanOfferSignInForPromos()` passes, and that only checks
`prefs::kSigninAllowed`. The pref is on for users who allow Google login for
extensions. Brave's `SHOULD_SHOW_SIGNIN_PROMO_COMMON` hook doesn't cover this
path. The pill shows in regular windows when there's more than one profile.

- [x] `chromium_src/chrome/browser/signin/signin_promo_util.cc`: make
      `ComputeProfileMenuAvatarButtonPromoInfo()` run its callback with an empty
      `ProfileMenuAvatarButtonPromoInfo`, as `ShouldShowExtensionSignInPromo` is
      already stubbed. Done as a plaster
      (`rewrite/chrome/browser/signin/signin_promo_util.cc.yaml`), which also
      replaced the `chromium_src` override
- [x] Add a test with `kSigninAllowed` on that checks the avatar button never
      enters the promo state

### Presubmit skip list for tsconfig parse errors is dead

Upstream split `CheckParseErrors` into `CheckJSONParseErrors` and
`CheckIDLParseErrors`
([878187ee59f9c](https://chromium.googlesource.com/chromium/src/+/878187ee59f9c)).
Brave's `per_check_files_to_skip` is keyed by check name and isn't validated, so
its entry now matches nothing. The next change to any of the 20 tsconfig files
with trailing commas (e.g. `tsconfig.json`, `build/tsconfig.json`) fails
presubmit.

- [x] `chromium_presubmit_config.json5:103`: rename `CheckParseErrors` to
      `CheckJSONParseErrors`
- [x] `chromium_presubmit_config.json5:172`: drop `CheckRawPtrUsage`, deleted
      before this range
      ([190dd427e740a](https://chromium.googlesource.com/chromium/src/+/190dd427e740a))
- [x] `script/chromium_presubmit_overrides.py` `setup_per_check_file_filter()`:
      raise on keys that name no existing check, as `ApplyBanRuleExcludes`
      already does for ban rules. Done in `PRESUBMIT.py`
      (`ValidatePerCheckFilesToSkip()`), since `build/PRESUBMIT.py` installs the
      filter without Chromium's checks; also dropped the dead
      `CheckNoBannedFunctions` key

### Windows-only FontPrewarmer browser test doesn't compile

`InProcessBrowserTest::browser()` returns a `BrowserWindowInterface*`, which has
no `profile()`, and `browser.h` is now visibility-restricted
([1cd5314fc9a7e](https://chromium.googlesource.com/chromium/src/+/1cd5314fc9a7e)).
The test was added during the cr156 rebase, but it is only built under `is_win`
(`test/BUILD.gn:1261`), so Linux builds don't see it.

- [x] `browser/brave_font_prewarmer_tab_helper_browsertest.cc`: use
      `GetProfile()` (lines 64, 89, 98) and `browser()->GetTabStripModel()`
      (line 86), and drop the `browser.h` include (line 12), as in
      [4beec1d68c91f](https://chromium.googlesource.com/chromium/src/+/4beec1d68c91f)

### Android Tips opt-in promo and notifications are on by default

Upstream launched `kAndroidTipsNotifications`
([5b50933d3803f](https://chromium.googlesource.com/chromium/src/+/5b50933d3803f)).
`TabbedRootUiCoordinator` now calls `TipsUtils.maybeShowTipsOptInPromo()`, which
shows a Chrome tips bottom sheet (Google Lens, Enhanced Safe Browsing, …). That
path doesn't depend on the segmentation platform Brave disables, and Brave has
no override.

- [x] `rewrite/components/segmentation_platform/public/features.cc.yaml`: ship
      `kAndroidTipsNotifications` disabled, and add it to
      `base/compile_overridden_features.inc`
- [x] `app/feature_defaults_unittest.cc`: add it to the Android disabled list
- [x] Consider plastering the Java safe default in `ChromeFeatureList.java`

### Send-tab-to-self device picker shows Google's "Manage your devices" link

Upstream enabled `kSendTabToSelfEnhancedBottomsheet` on Android
([4fa79c1018f77](https://chromium.googlesource.com/chromium/src/+/4fa79c1018f77)).
The new `EnhancedTargetDevicePickerView` has its own footer linking to the
Google account device page. Brave only hides the legacy
`ManageAccountDevicesLinkView` (bytecode super swap), so Brave Sync users with
another device see the link.

- [x] Ship `kSendTabToSelfEnhancedBottomsheet` disabled
      (`components/send_tab_to_self/features.cc` plaster plus
      `base/compile_overridden_features.inc`), or hide `manage_devices_block` in
      the new view. Product wants the enhanced sheet, so it stays enabled and a
      Java plaster on `EnhancedTargetDevicePickerView` hides
      `manage_devices_block` instead
- [x] Same commit: decide whether to keep `kSendTabToSelfExtraEntryPoints`
      ("Send to your devices" in the tab grid and toolbar long-press menus);
      Brave's tests only disable it. Kept: its entry points open the enhanced
      sheet, which no longer shows the Google account link.

### Bookmark bar IPH is on by default

Upstream enabled `kIPHBookmarkBarSimplifiedFeature` with the bookmark bar
simplification
([a9417b8d27bf1](https://chromium.googlesource.com/chromium/src/+/a9417b8d27bf1)).
Brave already ships `kNtpSimplificationBookmarkBar` on (#38334), so the bar
auto-hides after showing on the NTP for a number of days, and now this IPH shows
too. It isn't in Brave's IPH disable list.

- [x] `rewrite/components/feature_engagement/public/feature_constants.cc.yaml`:
      disable `kIPHBookmarkBarSimplifiedFeature`, and add it to
      `app/feature_defaults_unittest.cc`
- [ ] Product: decide whether Brave wants the bookmark bar auto-hide at all

### Email alias bubble uses a stale field ID

Upstream now passes the field to
`AutofillExternalDelegate::DidAcceptSuggestion()` and stops reading
`last_query_`, which "can be stale", behind the enabled-by-default
`kAutofillUsePassedFormAndFieldIds`
([bf1633dbb56cf](https://chromium.googlesource.com/chromium/src/+/bf1633dbb56cf),
[0025db05ef5dc](https://chromium.googlesource.com/chromium/src/+/0025db05ef5dc)).
Brave's preempt still passes `last_query_.field_id` to
`BraveHandleSuggestion()`, which anchors the email alias bubble on that field.

- [x] `rewrite/components/autofill/core/browser/ui/autofill_external_delegate.cc.yaml:27`:
      pass the `field_id` parameter instead

### iOS skips upstream's WebSocket block for local and WebUI pages

Upstream added a `ws(s)://` block to `CreateLocalBlockingJsonRuleList()` for
`file://` and WebUI pages
([99f13df2716d2](https://chromium.googlesource.com/chromium/src/+/99f13df2716d2)).
Brave's override returns an empty rule list, so neither that rule nor the
existing http(s) subresource rule is installed.

The override was added so WebUIs could open external links (originally Meld buy
links in Wallet). Agreed with the iOS team: rather than a list of
`ignore-previous-rules` exceptions, which would be fragile, route each case
below through native (mojo) handlers on master, then drop the override.
`//ios/web` opens a `target=_blank` link or `window.open()` from a WebUI as a
WebKit popup, which upstream's rules block for `https`.

- [ ] Leo conversation frame (`chrome-untrusted://leo-ai-conversation-entries`
      in `chrome://leo-ai`): open these through the page handler (as
      `OpenURLFromResponse` already does for web sources) instead of
      `target=_blank`, under
      `components/ai_chat/resources/untrusted_conversation_frame/components/`:
  - [ ] markdown links and citations: `markdown_renderer/index.tsx:199`, `:213`
  - [ ] search widget result cards and footer link:
        `search_widget/search_widget.tsx:64`, `:87`, `:117`, `:317`
  - [ ] search query links: `assistant_response/tool_event_search.tsx:31`
- [ ] Leo rich search widget: its iframe document loads, but its scripts, styles
      and fetches from `prod.browser-ai-includes.s.brave.app` (and possibly the
      AI chat API host) match top URL `chrome://leo-ai` and would be blocked.
      Not a link, so it needs a decision: a narrow allow entry for its origin,
      or another way to host it (`assistant_response/rich_search_widget.tsx:87`)
- [ ] Wallet (`chrome://wallet`): on iOS `chrome.tabs` is undefined, so these
      use `window.open()`. Route them through a native handler, under
      `components/brave_wallet_ui/`:
  - [ ] block explorers (arbitrary, user-editable URLs):
        `utils/block-explorer-utils.ts:151`
  - [ ] Meld buy widget: `page/screens/buy/hooks/use_buy.ts:555`
  - [ ] Meld terms of use:
        `components/desktop/popup-modals/partners_consent_modal/partners_consent_modal.tsx:46`
  - [ ] help center: `page/components/wallet_menus/wallet_settings_menu.tsx:109`
  - [ ] `openTab()` and its callers: `utils/routes-utils.ts:409`
  - [ ] static `target=_blank` links in `account-settings-modal.tsx`,
        `add-imported-account-modal.tsx`, `enable-nft-discovery-modal.tsx`,
        `hardware-wallet-connect/accounts_list.tsx`, `disclosures.tsx`,
        `address-message.tsx`, `checksum_info.tsx` and `privacy-modal.tsx`
- [ ] Then drop `chromium_src/ios/web/web_state/ui/wk_content_rule_list_util.mm`
      so upstream's http(s), popup and WebSocket rules apply again, and make
      sure upstream's `wk_content_rule_list_util_unittest.mm` and
      `wk_content_rule_list_provider_unittest.mm` run and pass for Brave iOS

Not affected: Wallet NFT, market and chart iframes load images through
`chrome-untrusted://image`, which the rules don't block. `chrome://leo-ai`,
`chrome://account`, `ads-internals` and `skus-internals` already open links
natively or have none.

### Android crash reports and sponsored media get the short version

Upstream made the manifest the only source of the version
([7cb5ec9c41e7f](https://chromium.googlesource.com/chromium/src/+/7cb5ec9c41e7f)).
Brave's plasters revert the C++ half, but the same commit removed
`VersionConstants.PRODUCT_VERSION`, so `VersionInfo.getProductVersion()` now
returns Brave's `versionName` (`1.98.13`) instead of `156.1.98.18`.

- [ ] Confirm which format the crash server and the sponsored media page expect
- [ ] If the 4-part one, restore it for these callers:
  - [ ] `components/minidump_uploader/android/java/src/org/chromium/components/minidump_uploader/util/BraveHttpURLConnectionFactoryImpl.java:22`
  - [ ] `android/java/org/chromium/chrome/browser/ntp/SponsoredRichMediaWebView.java:60`

### Isolated mode profiles missed by some private-window checks

Upstream made `IsEnterpriseIsolatedModeProfile()` mutually exclusive with
`IsIncognitoProfile()`
([4d3004e5c8b22](https://chromium.googlesource.com/chromium/src/+/4d3004e5c8b22)).
The rebase updated most call sites. These still treat isolated profiles as
normal ones. The impact is low, since isolated mode needs enterprise policy.

- [x] `chromium_src/chrome/browser/ssl/https_upgrades_util.cc:19`
      (`NormalWindowHttpsOnly`)
- [x] `chromium_src/chrome/browser/ui/autofill/chrome_autofill_client.cc:50` and
      `:101` (`kBraveAutofillPrivateWindows` gate)
- [x] `browser/ui/webui/ads_internals/ads_internals_ui.cc:170`
- [x] `browser/p3a/p3a_core_metrics.cc:45`

## 2. Upcoming deprecations

### ICU locale test scoper and `GetConfiguredLocale()`

`base::test::ScopedRestoreICUDefaultLocale` is deprecated in favour of
`base::i18n::ScopedDefaultIcuLocale`
([d9861dbd206d0](https://chromium.googlesource.com/chromium/src/+/d9861dbd206d0)).
Use
`const base::i18n::ScopedDefaultIcuLocale scoped_locale(base::i18n::GetKnownLanguageTag("en-US"));`
with `//base/i18n:test_support`
([b1ffc2ef2fb87](https://chromium.googlesource.com/chromium/src/+/b1ffc2ef2fb87)).
Upstream is also moving `GetConfiguredLocale()` to `GetDefaultIcuLocale()`
([aaccba3866072](https://chromium.googlesource.com/chromium/src/+/aaccba3866072)).

- [x] `components/brave_ads/core/internal/common/time/time_formatting_util_unittest.cc:23`,
      `:35`, `:46`, `:59`
- [x] `browser/brave_vpn/win/brave_vpn_wireguard_service/resources/resource_loader.cc:49`
      (`GetConfiguredLocale()`). Skipped: the service never sets the ICU locale,
      so `GetDefaultIcuLocale()` would always return `en-US` and drop its
      localization. Revisit when upstream removes `GetConfiguredLocale()`

### `BubbleDialogDelegate::set_shadow()`

`set_shadow()` is deprecated in favour of `set_shadow_config()`, which also
takes the elevation before the frame view exists
([03acd6864ec4c](https://chromium.googlesource.com/chromium/src/+/03acd6864ec4c)).

- [x] `browser/ui/views/brave_help_bubble/brave_help_bubble_delegate_view.cc:151`
- [x] Optional: replace `bubble_border()->set_md_shadow_elevation()` after
      creation with `.elevation`:
  - [x] `browser/ui/views/sidebar/sidebar_add_item_bubble_delegate_view.cc:114`
  - [x] `browser/ui/views/sidebar/sidebar_edit_item_bubble_delegate_view.cc:52`
  - [x] `browser/ui/views/sidebar/sidebar_item_added_feedback_bubble.cc:43`

### Renderer-initiated `OpenURLParams` will need an initiator state

`page_navigator.h` has a TODO to require `initiator_navigation_state` for
renderer-initiated navigations, and adds
`OpenURLParams::CreateRendererInitiated`
([aa1afc2a724b4](https://chromium.googlesource.com/chromium/src/+/aa1afc2a724b4)).
Brave builds these params browser-side with `is_renderer_initiated = true`.

- [ ] `browser/brave_ads/ads_service_delegate.cc:57`
- [ ] `browser/ui/webui/new_tab_takeover/android/new_tab_takeover_ui.cc:171`

Blocked until crbug.com/510258191 is enforced: neither site has a frame to take
a state from (the ad opens from a notification; the NTT WebUI's state would make
a `chrome://` document the initiator of another tab's navigation). Both rely on
renderer-initiated for Android external-app handling, which will need another
mechanism then.

### iOS `ApplicationLocaleStorage::Get()` → `GetTag()`

Upstream is migrating callers before removing `Get()`
([0de7fece6adbe](https://chromium.googlesource.com/chromium/src/+/0de7fece6adbe)).
Use `GetApplicationLocaleStorage()->GetTag().tag_string()`.

- [x] `ios/browser/api/brave_shields/adblock_service.mm:137`
- [x] `ios/browser/api/web_view/autofill/brave_web_view_autofill_client.mm:44`.
      Kept on `Get()`: `AutofillClient::GetAppLocale()` returns a
      `const std::string&`, and upstream's `ChromeAutofillClientIOS` still uses
      `Get()` for the same reason
- [x] `ios/browser/application_context/brave_application_context_impl.mm:68`
- [x] `ios/browser/safari_data_import/safari_data_importer_coordinator.mm:66`

## 3. Code-health migrations

### `GetDeprecatedID()` → `ChildProcessId`

Upstream is moving process IDs to the typed `ChildProcessId`
([6f7ff82eb0735](https://chromium.googlesource.com/chromium/src/+/6f7ff82eb0735)).

- [x] `browser/test/webui_subdomain_browsertest.cc:319`, `:324`: use `GetID()`

### iOS Global Privacy Control set by `//ios/web`

`//ios/web` now sets `globalPrivacyControlEnabled` itself from
`WebClient::GetUniversalOptOutState()`, and notes that setting it to false is
unsupported and can crash on early iOS 27 betas
([45b10a4381e7b](https://chromium.googlesource.com/chromium/src/+/45b10a4381e7b),
[54ecad77f3eef](https://chromium.googlesource.com/chromium/src/+/54ecad77f3eef)).
Brave sets it on every navigation, including to false.

- [ ] `ios/browser/web/brave_web_client.mm`: override
      `GetUniversalOptOutState()` from Brave's GPC pref
- [ ] `chromium_src/ios/web/navigation/crw_wk_navigation_handler.mm:38`: drop
      the per-navigation setter

Handed to the iOS team. `//ios/web` can't turn GPC back off once it's set in the
WebView configuration, while Brave's pref can be toggled at runtime, so the
per-navigation setter can't simply be dropped.

## 4. Obsolete Brave code

### `ViewShadow::OnLayerRecreated` workaround

Upstream deleted the same workaround from `ui/views/view_shadow.cc`: `View` now
swaps in the recreated layer before observers run
([2c8523a907b8d](https://chromium.googlesource.com/chromium/src/+/2c8523a907b8d)).
Brave's copy never finds the old layer any more.

- [x] `browser/ui/views/view_shadow.cc:107`: remove the `OnLayerRecreated()`
      override and `layer_owner_observation_`, then check the window-close
      animation by hand

### Single-process `ScopedChromeExtensionsClient` sharing

Upstream stopped registering a second extensions client in single-process mode
([c82bcddfb1391](https://chromium.googlesource.com/chromium/src/+/c82bcddfb1391)),
which is the problem Brave's sharing worked around.

- [x] `rewrite/chrome/common/scoped_chrome_extensions_client.cc.yaml`: drop the
      second and third substitutions, and the helpers in
      `chromium_src/chrome/common/scoped_chrome_extensions_client.cc:18-58`.
      Keep `rename_class`, and re-run the `--single-process`
      `brave_navigator_devicememory_farbling_browsertest.cc`

### Offer notification promo-code exclusion

`UpdateOfferNotificationVisibility()` now returns early unless two
disabled-by-default Wallet offer flags are on, and it was the only caller of
`ValidOfferExistsForUrl()`
([c0d7c639c3493](https://chromium.googlesource.com/chromium/src/+/c0d7c639c3493)).

- [ ] Drop
      `rewrite/components/autofill/core/browser/payments/offer_notification_handler.cc.yaml`
      and
      `chromium_src/components/autofill/core/browser/payments/offer_notification_handler.cc`,
      or keep them deliberately as a pin

### `layout_manager_base.cc` plaster

The plaster keeps a copy of the proposed layout because
[CL 8340933](https://chromium-review.googlesource.com/c/chromium/src/+/8340933)
made `GetProposedLayout()` return a reference. The CL was reverted, and its
reland returns by value again
([f8e1d7701b883](https://chromium.googlesource.com/chromium/src/+/f8e1d7701b883)),
as it does at 155 and 157, so the plaster now matches upstream's one-liner.

- [x] Delete `rewrite/ui/views/layout/layout_manager_base.cc.yaml` and its patch

### Test filters for tests upstream removed or renamed

A filter entry that no longer names a test does nothing. For renamed tests, the
new test runs unfiltered, so retarget rather than delete.

- [ ] Delete entries for tests upstream removed with fenced frames
      ([2b99b8b058be5](https://chromium.googlesource.com/chromium/src/+/2b99b8b058be5),
      [55fa5f273aed6](https://chromium.googlesource.com/chromium/src/+/55fa5f273aed6)):
      most of `test/filters/browser_tests.filter:1062-1138` (all but
      `TabStatsTrackerSubFrameBrowserTest`), `browser_tests-win.filter:173`,
      `content_unittests.filter:79`, `:84`, `:86`, and `unit_tests.filter:842`,
      `:845`
- [ ] Delete entries for removed Related Website Sets tests
      ([ad2d87d0a279c](https://chromium.googlesource.com/chromium/src/+/ad2d87d0a279c),
      [b749bc4d79b8c](https://chromium.googlesource.com/chromium/src/+/b749bc4d79b8c),
      [8385c578a8728](https://chromium.googlesource.com/chromium/src/+/8385c578a8728)):
      `browser_tests.filter:874`, `:1571`, `:1572`,
      `net_unittests.filter:80-83`, `unit_tests.filter:200-202`
- [ ] Delete the remaining entries for tests removed in range, e.g.
      `browser_tests.filter:897`, `:1648`, `:3018`
- [ ] `browser_tests-linux.filter:77-84`: delete
      `DevToolsProcessPerSiteTest.PausedDebuggerFocus`. The test is
      parameterised, so the entry never matched, and upstream fixed the cited
      flake
      ([d24197a6db520](https://chromium.googlesource.com/chromium/src/+/d24197a6db520))
- [ ] Retarget entries for renamed tests:
  - [ ] `browser_tests.filter:1361` →
        `All/GlicWebDragAndDropBrowserTest.testWebToGlicDragMaterialization*`
        ([5e09e48295972](https://chromium.googlesource.com/chromium/src/+/5e09e48295972))
  - [ ] `browser_tests.filter:2064` →
        `All/AppBrowserDocumentPictureInPictureBackendTest.ResizeToRespectsMaximumWindowSize/*`
        ([1f91ce49ddab6](https://chromium.googlesource.com/chromium/src/+/1f91ce49ddab6))
  - [ ] `unit_tests.filter:329` →
        `PaymentsChurnedUsersUiDelegateDesktopTest.ShowPaymentsChurnedUsersUI_WithAccountInfo`
        ([2f91f2ba60509](https://chromium.googlesource.com/chromium/src/+/2f91f2ba60509))
  - [ ] `browser_tests.filter:2809` `BookmarkBrowsertest.OpenAllBookmarks`:
        split and de-flaked upstream
        ([8ab47e637bab7](https://chromium.googlesource.com/chromium/src/+/8ab47e637bab7));
        drop the entry and watch CI
  - [ ] Delete, since the new names are already filtered:
        `browser_tests.filter:726-727`
        ([40151b7daac01](https://chromium.googlesource.com/chromium/src/+/40151b7daac01))
        and `browser_tests-linux.filter:47`
        ([56f48e0db9815](https://chromium.googlesource.com/chromium/src/+/56f48e0db9815))

### Leftover `prefs` binding on the Do Not Track toggle

`settings-do-not-track-toggle` became Lit and binds `pref-key`; it has no
`prefs` property
([ca41f56fcdba8](https://chromium.googlesource.com/chromium/src/+/ca41f56fcdba8)).

- [x] `browser/resources/settings/brave_privacy_page/brave_personalization_options.html:128`:
      drop `prefs="{{prefs}}"`. Also moved the `hr` class Brave adds to the
      toggle into a lit_mangler: adding it from the parent's `ready()` races the
      Lit element's first render

## 5. Optional

- [ ] Use `RenderFrameHost::ConsumeTransientUserActivation()`, now preferred for
      gating privileged actions
      ([233be19579106](https://chromium.googlesource.com/chromium/src/+/233be19579106)),
      in `browser/ui/webui/ai_chat/ai_chat_untrusted_conversation_ui.cc:352`
      (`OpenURL` only checks activation, so one gesture can open several tabs).
      First check that the renderer doesn't consume the activation itself.
- [ ] Farble `new VideoFrame(canvas).copyTo()`, which reads canvas pixels
      without going through Brave's canvas hooks, and now also covers
      null-format frames
      ([8dee2e87b7eea](https://chromium.googlesource.com/chromium/src/+/8dee2e87b7eea),
      [02d9bc78f7290](https://chromium.googlesource.com/chromium/src/+/02d9bc78f7290)).
      Perturb the snapshot in the canvas branch of `VideoFrame::Create`.
- [ ] Ship `kRendererAccessibleHttpCache` disabled and test it. Renderers would
      read cached subresources without going through
      `WillCreateURLLoaderFactory`
      ([3c5ebbd478b84](https://chromium.googlesource.com/chromium/src/+/3c5ebbd478b84),
      [dfee9bacb087a](https://chromium.googlesource.com/chromium/src/+/dfee9bacb087a)).
      It's off upstream and has no Blink consumer yet.
- [ ] Add `BraveExtensionsClient::IsCapturableURL()` returning false for
      `skus::IsSafeOrigin()`, using the hook upstream added
      ([137aa13aa45ca](https://chromium.googlesource.com/chromium/src/+/137aa13aa45ca)).
      `pageCapture.saveAsMHTML` doesn't consult `IsScriptableURL`, so an
      extension can snapshot account.brave.com.
- [ ] Anchor a closing first-in-group vertical tab to its group header, as
      upstream now does
      ([3217685fc72bc](https://chromium.googlesource.com/chromium/src/+/3217685fc72bc)),
      in `browser/ui/views/tabs/brave_tab_container.cc:285`.
- [ ] The show-avatar-menu shortcut does nothing while Brave hides the avatar:
      the profile menu now finds its anchor only through the element tracker
      ([29ca11b7935b9](https://chromium.googlesource.com/chromium/src/+/29ca11b7935b9)).
      Add a fallback anchor, or drop the shortcut.
- [ ] Replace `ABSL_FALLTHROUGH_INTENDED` with `[[fallthrough]]` and
      `ABSL_ARRAYSIZE` with `std::size()`. Abseil turned `ABSL_DEPRECATED` into
      a build error
      ([cc8895d9e1302](https://chromium.googlesource.com/chromium/src/+/cc8895d9e1302)),
      and these are documented as deprecated:

  - [ ] `components/brave_rewards/core/engine/wallet_provider/uphold/uphold_transfer.cc:122`,
        `:152`
  - [ ] `components/brave_rewards/core/engine/zebpay/zebpay.cc:65`
  - [ ] `components/brave_wallet/renderer/js_ethereum_provider.cc:835`, `:853`

- [ ] Drop test filters for tests upstream fixed or disabled itself, after a CI
      run:
  - [ ] `browser_tests.filter:1602`
        `All/PDFExtensionScrollTest.WithArrowLeftRightScrollToPage/*`, de-flaked
        ([0d412c91aa376](https://chromium.googlesource.com/chromium/src/+/0d412c91aa376))
  - [ ] `browser_tests.filter:2646`
        `HttpsUpgradesSafeBrowsingTest.SafeBrowsingBlock_ShouldNotTriggerHFM`,
        disabled upstream
        ([1f3b8f1648fdc](https://chromium.googlesource.com/chromium/src/+/1f3b8f1648fdc))

## For privacy review

- `kDevToolsAiNaturalLanguageInterface` (new, disabled), next to the DevTools AI
  flags Brave pins off.
- `kCustomizeChromeJourney`, `kReadAnythingJourney`, `kTabSearchJourney`
  (enabled): critical-user-journey UMA.
- `kSyncSimplifyDeviceNaming`, `kSyncUseServerDeterminedDeviceName`,
  `kSyncSessionsUsePreferredDisplayName` (enabled): device names over Brave
  Sync.
- `kSendTabToSelfAutoOpen`, `kSendTabToSelfPostSendToast`,
  `kSendTabToSelfEnhancedDesktopUI` (enabled): send-tab-to-self UX over Brave
  Sync.
- `kContextualTasksWebUiVoiceSearchDesktopAndroid` (enabled), under
  `kContextualTasks`, which Brave pins off.
