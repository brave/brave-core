# M158 code health follow-ups

Tasks found by reviewing upstream changes in `157.0.8103.3..158.0.8115.1` (6050
commits) for regressions, deprecations and code-health migrations that apply to
Brave. Tick items off as they land.

## 1. Regressions

### Brave's tab helpers are no longer attached to tabs

Upstream deleted `chrome/browser/ui/tab_helpers.{h,cc}`,
`TabAndroid::AttachTabHelpers` and every remaining `AttachTabHelpers` call site
([33087c2694270](https://chromium.googlesource.com/chromium/src/+/33087c2694270886eff8bdefcb953ae6e93bbec7)).
"[cr158] Drop the tab_helpers patch and override" removed the
`BRAVE_TAB_HELPERS` hook and says `brave::AttachTabHelpers()` "is no longer
called until it is migrated to BraveTabFeatures". Nothing calls it now
(`git grep -n 'AttachTabHelpers('`), on any platform. Tabs lose Shields
(`BraveShieldsWebContentsObserver`, `BraveShieldsTabHelper`),
`EphemeralStorageTabHelper`, the Brave `PageSpecificContentSettings` delegate,
Tor and Onion-Location, Wallet, AI Chat, Speedreader, Playlist, Wayback, News,
Web Discovery, DRM, PerfPredictor, SERP metrics, `CreativeSearchResultAd` and
geolocation helpers; on Android also Ads, Rewards and the YouTube injector.

Upstream also moved `CreateSessionServiceTabHelper()` out of
`AttachTabHelpers()`
([420714afe4097](https://chromium.googlesource.com/chromium/src/+/420714afe4097abadd140d8f7159954a829cd78e)),
so the Ads and Rewards helpers, which read `SessionTabHelper::IdForTab()` in
their constructors, have to be created after it.

- [x] Desktop Ads and Rewards: "[cr158] Attach tab helpers from
      `BraveTabFeatures::Init` on desktop", with browser tests
- [x] Desktop: `BraveTabFeatures::Init` calls `brave::AttachTabHelpers()`, and
      `OnWillDiscardContents` calls it on the replacement contents
- [ ] `browser/brave_tab_helpers.cc:110` `AttachTabHelpers()`: move the helpers
      into `BraveTabFeatures::Init` (`browser/ui/tabs/brave_tab_features.cc`),
      following the per-helper list and idiom in M157's "TabHelpers →
      TabFeatures"; the call from `Init` is only a stopgap
- [x] Android: same, in `browser/android/brave_tab_features.cc:35`, including
      Ads and Rewards. "[cr158][Android] Attach Brave tab helpers from
      BraveTabFeatures" calls `brave::AttachTabHelpers()` from the constructor,
      after `CreateSessionServiceTabHelper()`; the same stopgap as desktop
- [ ] `browser/ui/tabs/brave_tab_features.cc:121-122`: the other helpers follow
      discards through `OnWillDiscardContents`, but `rewards_tab_helper_` stays
      bound to the old contents; make it follow discards
      (`ContentsObservingTabFeature`, as `AdsTabHelper` already is)
- [ ] `browser/brave_tab_helpers.cc:192` `AttachPrivacySensitiveTabHelpers()`:
      decide which non-tab WebContents that upstream used to cover still need it
      (prerender, no-state prefetch, extension host, thin webview, overlay
      panel). Only AI Chat calls it now. M157 skipped moving
      `BraveShieldsWebContentsObserver` and `EphemeralStorageTabHelper` for this
      reason; tabs need them too now
- [ ] Add a browser test that a new tab has `BraveShieldsWebContentsObserver`
      and `EphemeralStorageTabHelper`, on desktop and Android
- [ ] Stale references: `browser/PRESUBMIT.py:27,99`,
      `docs/best-practices/architecture.md:1875-1902` (ARCH-074),
      `browser/ui/tabs/brave_tab_features.cc:108,116` and
      `browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_tab_transfer_bridge.cc:230`

### `settings-main` became Lit, so Brave's settings pages are no longer injected

`settings-main` now extends `CrLitElement`
([27dedb341b69](https://chromium.googlesource.com/chromium/src/+/27dedb341b6920c473f03e59011992e1b8010ae7)).
`polymer_overriding` only hooks Polymer elements, so the
`RegisterPolymerTemplateModifications({'settings-main': …})` call in
`browser/resources/settings/br/settings_main.ts` is a silent no-op. Brave's
pages (get started, origin, content, shields, web3, leo, sync, extensions,
system) aren't added to `#switcher`, the privacy page isn't forced visible, and
the performance page isn't removed. Upstream's template is now
`cache(this.showPage_(this.pageVisibility_?.X) ? html\`<div slot="view" id="X">…\`)`, and `renderPlugin\_`
takes only a route.

- [x] Fixed by "[cr158][settings] Port settings-main override to Lit"
      (`settings_main/settings_main.html.lit_mangler.ts`); `br/settings_main.ts`
      keeps only the page imports
- [x] `pageVisibility_` carries Brave's keys: the mangler reads
      `pageVisibility_?.{getStarted,origin,content,shields,braveWallet,…}`, all
      in `br/page_visibility.ts`
- [ ] The Polymer override forced `#privacy`'s template to always render; the
      Lit port doesn't override `renderPlugin_(routes_.PRIVACY)`. Confirm that's
      no longer needed
- [x] `br/settings_ui.ts:156-161`: `this.$.main` as the scroll target. Closed:
      upstream still declares `$.main` on `<settings-main id="main">`, and
      Brave's CSS gives `#main` `overflow: auto`
- [x] Re-check the Brave wrapper in
      `chromium_src/chrome/test/data/webui/settings/settings_main_test.ts`.
      Closed: it only imports the upstream test and adds the `BraveOriginPage`
      suite

### brave://history shows the Google history sync promo

[7ac98441fb0ba](https://chromium.googlesource.com/chromium/src/+/7ac98441fb0ba9c2d6b66ab5e53e1335e22d43b4)
enabled `syncer::kUnoPhase2FollowUp` on desktop. `history-app` then shows
`<history-sync-promo>` unless history sync is disabled or already on, and the
desktop `BrowsingHistoryHandler::ShouldShowHistoryPageHistorySyncPromo()` only
checks the shown and dismissed counts. A signed-out Brave profile reports
history sync as `kTurnedOff`, so the promo shows up to five times, and its
button starts Google sign-in. Brave pins nothing here, and no Brave file hides
the element.

- [ ] `chrome/browser/ui/webui/history/browsing_history_handler.cc`: plaster the
      desktop `ShouldShowHistoryPageHistorySyncPromo()` to run the callback with
      `false`, as upstream's Android branch does. Pinning `kUnoPhase2FollowUp`
      off also works, but other sign-in code reads that flag
- [ ] Add a test that brave://history doesn't render `history-sync-promo` for a
      signed-out profile
- [ ] `test/filters/unit_tests.filter:765-766`: the "we don't show the history
      sync promo" comment holds only once the hook lands

### Reading mode chip shows in the omnibox next to Speedreader

[8900faa0c3ff](https://chromium.googlesource.com/chromium/src/+/8900faa0c3ffe0f9efe90562109a91deadb3445a)
enabled `features::kReadAnythingOmniboxChip`. `ReadAnythingController`, which
`TabFeatures::Init` creates for every tab, then creates a
`ReadAnythingOmniboxController`, and distillable pages get upstream's "Reading
mode" page action, suggestion chip and IPH. Brave removes every other Reading
mode entry point in favour of Speedreader (`brave_app_menu_model.cc:434`,
`AppendReadAnythingItem()` in
`chromium_src/chrome/browser/renderer_context_menu/render_view_context_menu.h:51`,
`kShowReadAnything` in `customize_toolbar/list_action_modifiers.cc:119`), and
already filters `ReadAnythingOmniboxControllerBrowserTest`.

- [ ] `rewrite/ui/accessibility/accessibility_features.cc.yaml` (new): pin
      `kReadAnythingOmniboxChip` disabled
- [ ] `app/feature_defaults_unittest.cc`: add it

### Brave's site-settings default radio groups throw

`settings-category-default-radio-group` now keeps its selection in a synthetic
pref, `synthetic.settings.category_default.<category>`
([45c7ee7e63419](https://chromium.googlesource.com/chromium/src/+/45c7ee7e63419a212e84113280299c9a0a4918ba)).
`PrefService` creates those only for the 38 categories in
`CATEGORY_DEFAULT_SYNTHETIC_PREFS`
(`chrome/browser/resources/settings_shared/prefs2/pref_service.ts:16`), and
`getPref()` and `setSyntheticPref()` assert on anything else. Brave's categories
(`autoplay`, `googleSignIn`, `ethereum`, `solana`, `cardano`, `braveOpenAIChat`,
from
`patches/chrome-browser-resources-settings-site_settings-constants.ts.patch`)
aren't listed, so their default radios show no selection and don't save.

- [ ] Add Brave's categories to `CATEGORY_DEFAULT_SYNTHETIC_PREFS` with a
      plaster on `pref_service.ts`. Pages: `site_settings_autoplay.html`,
      `site_settings_google.html`, `site_settings_ethereum.html`,
      `site_settings_solana.html`, `site_settings_cardano.html` and
      `site_settings_brave_ai.html` in
      `browser/resources/settings/site_settings/`
- [ ] Add a WebUI test that each Brave default group shows its value and saves a
      click

### Tor bridges radio buttons throw

`settings-collapse-radio-button` moved to Lit on `PrefKeyObserverMixinLit`
([bff9d99b191c3](https://chromium.googlesource.com/chromium/src/+/bff9d99b191c3945e45272c60e60d4cd22dfc6ee)),
which asserts that `pref` is only ever set through `prefKey`.
`brave_tor_subpage.html:77,90,100` binds a fake `{}` pref on all three bridge
options, a workaround for the removed `PrefControlMixin`, so they throw in
`willUpdate`.

- [ ] `browser/resources/settings/brave_tor_page/brave_tor_subpage.html:77,90,100`:
      drop the `pref` bindings
- [ ] `browser/resources/settings/brave_tor_page/brave_tor_subpage.ts:135-142`:
      drop `useBridgesValuePref_` and its comment

### iOS: upstream's app-specific-pages rule list applies again

Upstream renamed `CreateLocalBlockingJsonRuleList()` to
`CreateAppSpecificURLsJsonRuleList()`, dropped `file://` from it and added the
`document` resource type
([e7777f06181e1](https://chromium.googlesource.com/chromium/src/+/e7777f06181e1e1a023dc309eee11da3bd9ec362)).
`chromium_src/ios/web/web_state/ui/wk_content_rule_list_util.mm` still defines
the old name to return an empty list, so it matches nothing and WebUIs are
blocked from loading external resources and documents again. The same commit
makes `favicon_util.mm` reject http(s) favicons on app-specific pages.

- [ ] Retarget the `#define` to `CreateAppSpecificURLsJsonRuleList`
- [ ] An iOS owner confirms that blocking `document` loads and http(s) favicons
      is acceptable on Brave's app-specific pages

### iOS: `GoogleGroupsManagerFactory` override no longer matches the header

A scripted series of about 30 commits converts `ProfileKeyedServiceFactoryIOS`
subclasses to `TypedProfileKeyedServiceFactoryIOS`
([3cc80ea5bcdc3](https://chromium.googlesource.com/chromium/src/+/3cc80ea5bcdc3),
Bug 570433691). `GoogleGroupsManagerFactory` now has
`explicit GoogleGroupsManagerFactory(PassKey)` and generated `GetInstance()` and
`GetForProfile()`, but
`chromium_src/ios/chrome/browser/metrics/model/google_groups_manager_factory.mm`
replaces the whole upstream file with the old default constructor and
hand-written accessors. This breaks the iOS build; Linux doesn't compile it.

- [x] Fixed by "[cr158][ios] Fix `GoogleGroupsManagerFactory` override for typed
      factory"
- [ ] `chromium_src/ios/chrome/browser/signin/model/about_signin_internals_factory.cc:15-31`:
      upstream converted `AboutSigninInternalsFactory` too
      ([61af71ca018a5](https://chromium.googlesource.com/chromium/src/+/61af71ca018a5150f0ea1f042637131d493656bf)),
      and Brave's full-file replacement still defines the default constructor,
      destructor, `GetForProfile()` and `GetInstance()`. Keep only the `PassKey`
      constructor, the null `BuildServiceInstanceFor()` and
      `RegisterProfilePrefs()`, as for `GoogleGroupsManagerFactory`. No other
      `chromium_src/ios` factory replaces the whole file

### iOS: `MockCredential` no longer conforms to `Credential`

The `Credential` protocol gained the required `securityDomainEncrypted` and
`deviceAuthKeyVersion` properties
([3c96d0a6c3f4](https://chromium.googlesource.com/chromium/src/+/3c96d0a6c3f4affd596eee0dfedebd9b4bd6b097)).
Brave's Swift `MockCredential` lists every property, so iOS Debug builds break
(as in M140, "[cr140][ios] Update `MockCredential` to account for new
`Credential` properties").

- [x] Fixed by "[cr158][ios] Update `MockCredential` for new `Credential`
      properties"

### iOS: `HistoryServiceFactory::GetForProfile()` lost its `ServiceAccessType`

`ios::HistoryServiceFactory` became a typed factory with no access traits, so
only `GetForProfile(ProfileIOS*)` exists
([615dca3eddd21](https://chromium.googlesource.com/chromium/src/+/615dca3eddd21fc5fe03471f70ef670c2f61781c)).
Brave's calls still pass an access type, which breaks the iOS build.
`FaviconServiceFactory` and the password store factories keep
`RejectImplicitAccessIfIncognito`, so their two-argument calls still compile.

- [ ] `ios/browser/api/history/brave_history_api.mm:216-217,225-226`
- [ ] `ios/browser/api/history/importer/brave_history_importer.mm:39-40`
- [ ] `ios/browser/safari_data_import/safari_data_importer_coordinator.mm:54-55`
- [ ] `ios/browser/ui/webui/ai_chat/ai_chat_ui.mm:186-187`
- [ ] `chromium_src/ios/chrome/browser/sync/model/sync_service_factory.mm:16-17`:
      upstream now calls `GetForProfile(profile)` there too. The
      `IMPLICIT_ACCESS` call never returns null now; check that
      `BraveSyncServiceImplDelegate` doesn't rely on null

### iOS: `CSPDirectiveName::TreatAsPublicAddress` removed

[62b4de517e442](https://chromium.googlesource.com/chromium/src/+/62b4de517e442f164d27559461fcfd1fa9f3616b)
removed the directive from the mojom enum.

- [ ] `ios/web/webui/brave_url_data_source_ios.mm:109`: drop the `case`

### iOS: `brave_providers` misses the embedded reauth provider

[969bd1233f6d](https://chromium.googlesource.com/chromium/src/+/969bd1233f6d8e9c9a8b678bb5c68270730910b6)
added `CreateEmbeddedReAuthInteractionManager()` and
`signin:chromium_embedded_reauth` to upstream's `chromium_providers`. Nothing
calls it yet; the follow-up CLs the commit names will, and Brave's app then
fails to link.

- [ ] `ios/browser/providers/BUILD.gn:57`: add
      `//ios/chrome/browser/providers/signin:chromium_embedded_reauth`

### `html_capability_element_base.cc` patch registers the control twice

The patch for `MaybeRegisterPageEmbeddedPermissionControl()` was adjusted by the
rebase, but upstream now computes `descriptor` before `Bind()`
([6563f89211c48](https://chromium.googlesource.com/chromium/src/+/6563f89211c484d3cfb7866d875045d806e505a4)).
Brave's hunk adds a second `CreateEmbeddedPermissionRequestDescriptor()` after
`Bind()`, which forces another layout, and its `kUserMedia` early `return false`
leaves a bound receiver. The second hunk also rewrites an upstream comment.

- [ ] `patches/third_party-blink-renderer-core-html-html_capability_element_base.cc.patch`:
      check `descriptor->detail->which() == kUserMedia` before `Bind()`, using
      the existing `descriptor`
- [ ] Drop the comment edit

### Component updater crashes on Brave's ECDSA CUP key

Upstream removed ECDSA CUP verification and `kPqcCupSigning`; `Cup::Cup()` now
does `CHECK(public_key->IsMldsa44())` and key IDs are `ML-DSA-44-<n>`
([b5589ebe1d1bd](https://chromium.googlesource.com/chromium/src/+/b5589ebe1d1bda6ca77af7ec34706e1317b6d7bd)).
Brave's plaster makes `BuildSigner()` return a `Cup` built from
`kBravePublicKey`, a P-256 SPKI. `RequestSender`'s constructor always builds the
signer, so the first component or extension update check hits the `CHECK`, even
though Brave doesn't sign requests.

- [ ] `chromium_src/components/update_client/request_sender.cc:14-26`: switch to
      a Brave ML-DSA-44 key (needs update server support), or stop building a
      signer from the ECDSA key. "[cr158][WIP] ECDSA CUP verification removed"
      only avoids the crash:
      `patches/components-client_update_protocol-cup.cc.patch` deletes the
      `IsMldsa44()` `CHECK`, and the key ID is now `ML-DSA-44-1` on an ECDSA
      key. Drop that patch with the fix
- [ ] `rewrite/components/update_client/request_sender.cc.yaml`: update the
      description, which still describes upstream's ECDSA/ML-DSA flag
- [ ] `chromium_src/components/update_client/request_sender_unittest.cc`: the
      `cup2key=1:` expectations and `BraveKeyIgnoresPqcCupSigningFlag` refer to
      the old format and the deleted flag
- [ ] `test/filters/components_unittests.filter:263-267`: re-evaluate
      `-All/RequestSenderTest.CupKeySelection/*` and its comment

### The geolocation permission prediction model is fetched through the component updater

Upstream enabled `kPredictionModelComponentDelivery` with `targets` = `20`
(`OPTIMIZATION_TARGET_GEOLOCATION_PERMISSION_PREDICTIONS`)
([42bb7e693b546](https://chromium.googlesource.com/chromium/src/+/42bb7e693b5466c5e0523d830b39298a55625b27)).
For a migrated target, `PredictionModelComponentUpdateListener` registers a
component with the component updater instead of asking the `PredictionManager`
that Brave stubs out. `ProfileImpl` warms up `PredictionModelHandlerProvider`
after startup on every regular profile, through
`OptimizationGuideGlobalStateHolderKeyedService` and not the Brave-gated
`OptimizationGuideKeyedService`. With
`kPermissionOnDeviceGeolocationPredictions` on, that registers "Optimization
Guide Geolocation Permission Predictions" (`kpfeefaldebcckpkaeelpiecbjfpbmle`).
Brave gets nothing from the model (`kPermissionPredictionsV2` is off).

- [ ] `chromium_src/components/component_updater/component_installer.cc:38`: add
      `kpfeefaldebcckpkaeelpiecbjfpbmle` to `kDisallowedComponents`, plus the
      other configured targets so a `targets` change can't enable them:
      `fmkegdflifakmolfcmpbohjhgpicfbol` (notifications),
      `omccndhloelimkeacgjfjaipjhnnogmb` (CSD phishing) and
      `chjfeldfbijdbdeojjhodnibjnaohpdh` (CSD image embedder). Or pin
      `kPredictionModelComponentDelivery` disabled in
      `components/optimization_guide/core/delivery/prediction_model_component_configs.cc`
      and add it to `app/feature_defaults_unittest.cc`

### Pre-existing: Translator and LanguageDetector web APIs are exposed

Brave's json5 plaster only ships `LanguageDetectionAPI` and `TranslationAPI`
with `base_feature_status: "disabled"`. Both entries have
`copied_from_base_feature_if: "overridden"`, so `UpdateStatusFromBaseFeatures()`
copies the base feature only when something overrides it, and the `stable`
status wins. `app/feature_defaults_unittest.cc` asserts the base features are
off and not overridden, which is exactly the state that leaves the APIs on.
Desktop has been like this since Brave's "[plaster]
`OVERRIDE_FEATURE_DEFAULT_STATES` migration VI" (3bca5e99fc7); this range made
`LanguageDetectionAPI` stable on Android too
([f2914fbac90d1](https://chromium.googlesource.com/chromium/src/+/f2914fbac90d1922e4aff799d85320c1d869d29b)).
`AIProofreadingAPI`, `AIRewriterAPI` and `AIWriterAPI` use the same mode but are
still experimental.

- [ ] Confirm in a build that `'LanguageDetector' in self` and
      `'Translator' in self` are true
- [ ] `renderer/brave_content_renderer_client.cc:149`: disable them with
      `EnableFeatureFromString`, as for `AIDecisionModelAPI`, or set the json5
      `status` instead of `base_feature_status`
- [ ] `app/feature_defaults_unittest.cc:367`: also check the runtime feature,
      not only the base feature

### "Relaunch to update" skips Brave's update dialog

Upstream removed `kFewerUpdateConfirmations`, and `MaybeUpdateChrome()` now
always calls `chrome::AttemptRelaunch()` on Windows, Mac and Linux
([206e587624a3d](https://chromium.googlesource.com/chromium/src/+/206e587624a3d40a381bf5250cb5f134c8aadae1)).
Brave had pinned the flag off because relaunching straight from the menu command
left the updated browser not starting or crashing (master 91f761e44c5); "[cr158]
Drop the kFewerUpdateConfirmations pin" dropped the pin without replacing it.

- [ ] Test "Relaunch to update" on Mac (Sparkle) and Windows. If the old failure
      is back, hook `chrome::MaybeUpdateChrome` to show the dialog
- [ ] `browser/ui/views/frame/brave_browser_view.cc:652`
      `ShowUpdateChromeDialog()`: unreachable on desktop; remove it with
      `browser/ui/views/update_recommended_message_box_mac.{h,mm}` and
      `patches/chrome-browser-ui-views-update_recommended_message_box.h.patch`,
      or re-wire it

### Hovered inactive tabs lose Brave's tab text colours

`TabStyle::GetTabForegroundColor()` now reads
`kColorTabForegroundInactiveHoverFrame{Active,Inactive}` for hovered inactive
tabs, which `AddTabStripColorMixer` defaults to the non-hover IDs
([33a6011aae094](https://chromium.googlesource.com/chromium/src/+/33a6011aae094df378af3489de54e8109207c8c0)).
References resolve through `GetLastNonPostprocessingMixer()`, and Brave sets the
non-hover IDs in a postprocessing mixer (70% opacity, and the darker theme), so
hovered tabs render without them.

- [ ] `browser/ui/tabs/brave_tab_color_mixer.cc:220-253`: set the two hover IDs
      in the postprocessing mixer, as for the non-hover ones
- [ ] `browser/ui/views/tabs/tab_style_views_unittest.cc`: add a hovered
      inactive tab case

### Trezor Connect popup loses `window.opener`

Upstream now treats "a `chrome-untrusted://` page opens a blank popup, then
navigates it to the web" as tabnabbing, and swaps the BrowsingInstance, which
severs the opener
([3e3f9be5c7da0](https://chromium.googlesource.com/chromium/src/+/3e3f9be5c7da0257315bb2a1cd501c9d8b50723f),
not behind a flag). Brave's Trezor bridge relies on exactly that path:
`components/brave_wallet_ui/trezor/trezor_patches.ts:16` rewrites
`window.open(url)` from `chrome-untrusted://trezor-bridge` into
`window.open('')` plus `.location.href = url`, so that Trezor Connect keeps an
opener to `postMessage` to. Trezor hardware wallet flows break.

- [x] Fixed by "[cr158] Keep opener for the Trezor popup opened from
      trezor-bridge", which exempts `chrome-untrusted://trezor-bridge` →
      `https://connect.trezor.io` in a `render_frame_host_manager.cc` plaster.
      The `window.open` proxy in `trezor_patches.ts` is still needed, and
      `CheckOpenerInPopup` stays as is

### iOS: deleting history crashes without Google history sync

`WebHistoryServiceFactory` now always creates the service, and every request
method `CHECK(IsEnabled())`. Upstream's drivers return the service only when
`IsEnabled()`
([0c613e80df545](https://chromium.googlesource.com/chromium/src/+/0c613e80df545d2451bc8d709d64636339bb1b68)).
Brave's `HistoryDriverIOS` is a fork of `IOSBrowsingHistoryDriver` and returns
the getter's result unconditionally, so `BrowsingHistoryService::RemoveVisits()`
(called at `ios/browser/api/history/brave_history_api.mm:303`) reaches
`WebHistoryService::ExpireHistory` and crashes.

- [x] `ios/browser/api/history/history_driver_ios.mm:65`
      `GetWebHistoryService()`: obsolete as of 8109.1. Upstream removed the
      driver getter
      ([40b8a4857367](https://chromium.googlesource.com/chromium/src/+/40b8a4857367b5f402578e83406784553840b212)),
      `BrowsingHistoryService` takes the service in its constructor and checks
      `IsEnabled()` itself, and the rebase commit "[cr158] Inject
      `WebHistoryService` into `BrowsingHistoryService`" dropped Brave's getter

### Pre-existing: iOS web pages can navigate a tab to the NTP URL

Upstream stopped web content from navigating to `about://newtab/` by making
`NewTabPageTabHelper` a policy decider that only allows browser-initiated
main-frame loads, reloads, back/forward and restores
([2765114be16e9](https://chromium.googlesource.com/chromium/src/+/2765114be16e97619339c6438095d82e027782f2)).
Brave's `tab_helper_util.mm` replacement never creates that helper, and Brave's
own NTP uses the same URL (`TabManager.ntpInteralURL`). Brave's decider allows
every `about:` request in any frame (`BVC+TabPolicyDecider.swift:112`), and
`isNewTabURL` then shows the native NTP over the page. An iOS owner should
confirm the visible effect.

- [ ] `ios/brave-ios/Sources/Brave/Frontend/Browser/BrowserViewController/BVC+TabPolicyDecider.swift:112`:
      allow `isNewTabURL` only for main-frame privileged requests, reloads,
      back/forward and restores
- [ ] Add a test that `location = 'about://newtab'` and
      `window.open('about://newtab')` from a page don't show the NTP

### Android: Brave's NTP toolbar side padding no longer applies

Upstream removed `OmniboxResourceProvider.getToolbarSidePaddingForNtp()`, and
`ToolbarPhone` now reads `R.dimen.toolbar_edge_padding_ntp` (16dp) directly
([78fd9a6f89e22](https://chromium.googlesource.com/chromium/src/+/78fd9a6f89e22cec8da707c8c536ed62d4158c46)).
Brave's `changeMethodOwner` redirect to `BraveOmniboxResourceProvider` (8dp,
`brave_toolbar_edge_padding_ntp`) matches nothing, and the `BytecodeTest`
`methodExists` check fails. This is what M158's "`OmniboxResourceProvider`
static methods" deprecation item expected.

- [ ] Restore the 8dp padding: plaster `mToolbarSidePaddingForNtp` in
      `chrome/browser/ui/android/toolbar/java/src/org/chromium/chrome/browser/toolbar/top/ToolbarPhone.java:360`
      to use `R.dimen.brave_toolbar_edge_padding_ntp`
      (`android/java/res/values/brave_dimens.xml:52`)
- [ ] Delete
      `build/android/bytecode/java/org/brave/bytecode/BraveOmniboxResourceProviderClassAdapter.java`,
      its entries in `build/android/bytecode/BUILD.gn:109` and
      `BraveClassAdapter.java:109`,
      `android/java/org/chromium/chrome/browser/omnibox/styles/BraveOmniboxResourceProvider.java`
      and `android/brave_java_sources.gni:297`
- [ ] `android/javatests/org/chromium/chrome/browser/BytecodeTest.java:1000-1006`:
      drop the assertion
- [ ] `android/java/apk_for_test.flags:974-976`: drop the keep rule

### Android: the Homepage row moved to Appearance

With `kAndroidBottomBar` on (its `move_toolbar_settings_to_appearance` param
defaults to true), the `homepage` row is inflated by
`appearance_preferences.xml` and `MainSettings` drops its own
([0a3ed81a508b3](https://chromium.googlesource.com/chromium/src/+/0a3ed81a508b3)).
Brave's homepage handling is keyed on the `MainSettings` row, so the Appearance
row opens upstream's `HomepageSettings`. With the flag off, Brave's copied
search index provider lacks upstream's new `removeEntryForKey(PREF_HOMEPAGE)`,
so settings search lists a Homepage row under Appearance that isn't there.

- [ ] `android/java/org/chromium/chrome/browser/settings/AppearancePreferences.java:84`:
      point the inflated row at `BraveHomepageSettings` and apply Brave's hide
      rule, as `BraveMainPreferencesBase.java:365,439,534` does for Main
- [ ] `AppearancePreferences.java:463-530` `SEARCH_INDEX_DATA_PROVIDER`: the
      `PREF_HOMEPAGE` removal landed in "[cr158][Android] Fix duplicate
      Appearance entries in the settings search index" (unconditionally, where
      upstream removes it only with the row hidden). Pre-existing, it still
      lacks the `PREF_TAB_POSITION` removal

### Android: `UndoBarController.showUndoBar(List, List, boolean)` removed

The method became the private `showUndoBar(UndoClosureSnapshot)`, and
`UndoActionData` was replaced by `UndoBarContent`
([7cc7f72c6058d](https://chromium.googlesource.com/chromium/src/+/7cc7f72c6058d93458399b4406a3a7a75dc9b9e1)).
`BraveUndoBarControllerBase` no longer compiles, its reflection call would
return null (no undo bar), and `changeMethodOwner` matches by name only, so the
new call gets redirected to a descriptor Brave lacks.

- [x] Fixed by "[cr158][Android] Replace the undo bar bytecode adapter with a
      plaster", which deletes `BraveUndoBarControllerBase`, its class adapter
      and the `BytecodeTest` assertion in favour of an `UndoBarController.java`
      plaster
- [ ] `android/java/proguard.flags:70-72`: the `showUndoBar(...)` keep rule only
      served the deleted reflection call; delete it

### Android: `OriginalProfileSupplier` removed

Upstream deleted the class rather than migrate it, since it assumes one regular
profile per process
([a1ca906126779](https://chromium.googlesource.com/chromium/src/+/a1ca906126779f4b0e37167dca13c48fb39deec1)).
Brave uses it to apply its default search engine once the regular profile
exists, which breaks the Android build.

- [ ] `android/java/org/chromium/chrome/browser/BraveApplicationImplBase.java:19,71`
- [ ] `android/java/org/chromium/chrome/browser/referrer/BraveReferrer.java:29,223-232`
- The replacement is a `ProfileManager.Observer` that acts on the first non-OTR
  `onProfileAdded()`, which is what the upstream class did

### Android: `GlicButtonDelegate` moved to `glic.toolbar_button`

[1b4c81d02f73b](https://chromium.googlesource.com/chromium/src/+/1b4c81d02f73b96529bac87a6ffe825b1a900c3a)
split `//chrome/browser/glic/android` into subpackages. `GlicEnabling` stayed,
and no Brave XML names a moved class.

- [ ] Update the import in
      `android/java/org/chromium/chrome/browser/tabbed_mode/BraveTabbedAdaptiveToolbarBehavior.java:20`,
      `android/java/org/chromium/chrome/browser/toolbar/BraveToolbarManager.java:53`,
      `android/javatests/org/chromium/chrome/browser/BytecodeTest.java:86` and
      `android/junit/src/org/chromium/chrome/browser/tabbed_mode/BraveTabbedAdaptiveToolbarBehaviorTest.java:31`

### Android: `product_config_java_packages` no longer exists

[fde64a8deb78f](https://chromium.googlesource.com/chromium/src/+/fde64a8deb78fdf0efdff5127444225cba6527a6)
folded `ProductConfig` into `BuildConfig`, and `ResourceBundle` reads
`BuildConfig.LOCALES` itself. GN fails on the unused assignment.

- [ ] `test/BUILD.gn:1661-1666`: drop the assignment and its comment

### Android: `AppMenu` constructor lost its `boolean` parameter

[a261b745aaa91](https://chromium.googlesource.com/chromium/src/+/a261b745aaa912b3ee4caa60d75adcb3fab8ab55)
removed `disableVerticalScrollbar` and
`AppMenuDelegate.shouldDisableVerticalScrollbar()`.

- [x] Fixed by "[Android][cr158] Removed disableVerticalScrollbar from
      AppMenu/ctor", including the `BytecodeTest` `constructorsMatch`

### Android: `TabSwitcherActionMenuCoordinator` takes a `TabGroupExistenceChecker`

The constructor and `createOnLongClickListener` take a
`TabGroupExistenceChecker` instead of a `TabWindowManager`; `ToolbarManager`
passes a lambda over `GroupWindowChecker.hasOtherGroups()`
([bdc2956b708e9](https://chromium.googlesource.com/chromium/src/+/bdc2956b708e9345e4cb51b9ca4af9d6cd2a5db2)).

- [x] Fixed by "[Android][cr158] Fix TabSwitcherActionMenu single-group Add/Move
      logic", covering both coordinators, `BraveToolbarManager` and the junit
      test

### Android: `BottomBarCoordinator` constructor gained five parameters

[a728af2b44c67](https://chromium.googlesource.com/chromium/src/+/a728af2b44c67)
added `toolbarPositionSupplier`, and
[af63bfedede38](https://chromium.googlesource.com/chromium/src/+/af63bfedede38)
added `browserControlsVisibilityManager`, `scrimManager`, `controlContainer` and
`bottomAppBarContainer`. Brave's `new Brave\1` plaster on
`BottomBarContainerCoordinator` already passes all 15 arguments, but
`BraveBottomBarCoordinator` still takes 10.

- [x] Fixed by "[Android][cr158] Wire BottomBarEducationCoordinator into
      BottomBarCoordinator"

### Android: `kPopularSitesJsonPref` moved to a private header

The popular sites prefs moved to
`components/ntp_tiles/popular_sites_pref_names.h`, whose target is visible only
inside `//components/ntp_tiles`
([6c240d5b67c85](https://chromium.googlesource.com/chromium/src/+/6c240d5b67c857fdd2239333b74a2701fdbcf461)).
Brave's override is likely redundant:
`chromium_src/components/ntp_tiles/most_visited_sites.cc` already forces
`popular_sites_` to null.

- [ ] `browser/brave_profile_prefs.cc:135,190-191`: drop the
      `kPopularSitesJsonPref` default and the include. "[Android][cr158] Ntp
      tiles pref names moved and restricted visibility" kept it by widening the
      target's visibility with a `components/ntp_tiles/BUILD.gn` plaster; drop
      that plaster and dep too

### Android: per-site Autoplay and Google Sign-In rows aren't set up

The private
`SingleWebsiteSettings.setupContentSettingsPreference(Preference, Integer, boolean, boolean)`
gained a `@Nullable PermissionSetting` parameter
([0c45e73d2543d](https://chromium.googlesource.com/chromium/src/+/0c45e73d2543de1db5f266a76d79b33753a9be02)).
Brave calls the old signature by reflection, so in release builds
`BraveReflectionUtil.invokeMethod` returns null and the rows stay empty.

- [x] Fixed by "[cr158][Android] New arg permissionSetting at
      SingleWebsiteSettings.setupContentSettingsPreference", including the
      `BytecodeTest` signature

### Mac universal builds drop one architecture's WebUI code cache

Upstream now ships the Mac WebUI code cache as one pak per architecture,
`webui_code_cache.{x86_64,arm64}.pak`
([9ff4db6058293](https://chromium.googlesource.com/chromium/src/+/9ff4db60582931d9b72f1b83185044dc59302935)).
`installer/mac/copy_other_arch_binary.py:41` skips `*.pak` when it copies the
other architecture's app, and `universalizer.py` keeps files present in only one
input, so a universal build ships one architecture without its cache. The pak is
optional: slower WebUI startup, no crash.

- [ ] `installer/mac/copy_other_arch_binary.py:41`: let `webui_code_cache.*.pak`
      through the ignore pattern

### Pre-existing: CORS preflights from .onion pages send the real Origin

Brave's plaster makes the Origin opaque for cross-origin requests from `.onion`
pages only in `CorsURLLoader::MaybeSetOriginHeader`
(`rewrite/services/network/cors/cors_url_loader.cc.yaml`). The preflight is
built in `PreflightController::CreatePreflightRequest` from
`tainted ? url::Origin() : *request.request_initiator`, and
`CorsURLLoader::StartPreflightCheck` passes `tainted_` alone. A non-simple
cross-origin request from a .onion page sends `Origin: http://<x>.onion` on the
`OPTIONS` preflight. Predates the range (found because
[2b7e971c1b740](https://chromium.googlesource.com/chromium/src/+/2b7e971c1b7405ba29765aa3c8fdb57f018b5a17)
touched `StartPreflightCheck`); a Tor owner should confirm the intent.

- [x] `rewrite/services/network/cors/cors_url_loader.cc.yaml`: also pass
      `tainted_ || IsCrossOriginOnionRequest(request_)` to
      `PerformPreflightCheck` in `StartPreflightCheck`

### Pre-existing: Brave's commander suggestion group is never registered

Upstream's `BuildDefaultGroups()` is in an anonymous namespace, and since M131
its only caller is `BuildDefaultGroupsForInput()` in the same file
([925c84622a5f9](https://chromium.googlesource.com/chromium/src/+/925c84622a5f9a0a753a3631c8e1b24f7bac5240)).
Brave's `#define BuildDefaultGroups BuildDefaultGroups_ChromiumImpl` renames
that call too, and nothing calls Brave's `omnibox::BuildDefaultGroups()`. So
`GROUP_OTHER_NAVS` never gets its `IDS_IDC_COMMANDER` header, and commander
matches fall back to `SECTION_DEFAULT` with no header.

- [ ] `chromium_src/components/omnibox/browser/suggestion_group_util.cc:11-29`:
      wrap `BuildDefaultGroupsForInput` instead, or plaster the group into
      upstream's `BuildDefaultGroups()`; drop the dead wrapper
- [ ] `components/omnibox/browser/commander_provider_unittest.cc:366`: also
      check that `GROUP_OTHER_NAVS` has a non-empty header

## 2. Upcoming deprecations

### `TOP_LEVEL_STORAGE_ACCESS` content setting

Upstream removed `document.requestStorageAccessFor()` and says the rest of the
`top_level_storage_access` code follows
([064bca2b1e8bc](https://chromium.googlesource.com/chromium/src/+/064bca2b1e8bc9dfdd45d6f6175b67df938e52b7)).
Removing the enum value will break the build.

- [ ] `chromium_src/components/content_settings/core/browser/content_settings_registry.cc:334-348`:
      drop Brave's re-registration of
      `ContentSettingsType::TOP_LEVEL_STORAGE_ACCESS` when the type goes

### Android: `OmniboxResourceProvider` static methods

The class now says "Existing static methods are set to be retired"; its public
statics fell from 54 to 20 in 8104.1..8107.2, to 19 at 8111.0 (e.g.
[49111387fe95e](https://chromium.googlesource.com/chromium/src/+/49111387fe95eb6cefd2465f4b93b74f021b2c39))
and to 18 at 8113.1
([41057f5c9f85](https://chromium.googlesource.com/chromium/src/+/41057f5c9f8588e3097f9f987460bf58a33848c7)).
Brave redirects the static `getToolbarSidePaddingForNtp(Context)` with
`changeMethodOwner`, which matches by name only, and an instance method of the
same name already exists. If upstream's caller switches to it, the redirect
targets a method Brave lacks; if the static goes, the pin is lost silently.

- [x] Closed at 8115.1: upstream removed the method, and the redirect broke; see
      "Android: Brave's NTP toolbar side padding no longer applies". Public
      statics are down to 11

### `ActorKeyedService::GetTaskFromTab()`

Deprecated in favour of
`GetTaskFromActorSurface(ActorSurfaceHandle::From(tab.GetHandle()))`
([54d881f04e4d4](https://chromium.googlesource.com/chromium/src/+/54d881f04e4d448280ba40978cc3c19cc3a7bab9)).

- [ ] `browser/ai_chat/ai_chat_conversation_task_browsertest.cc:409,413`

## 3. Code-health migrations

### `cbor::Writer::Write` without a temporary `cbor::Value`

Upstream is passing maps and arrays to `cbor::Writer::Write` directly through
`ValueView` (e.g.
[970f30a9b8ff7](https://chromium.googlesource.com/chromium/src/+/970f30a9b8ff76dc186b58a2ca0510643c1c0bda):
`cbor::Writer::Write(map)`). `Writer::Write(cbor::Value(` fell from 69 to 28 at
8104.1, and 25 at 8107.2 through 8115.1
(`git grep -E 'Writer::Write\(cbor::Value\(' <tag> -- '*.cc' '*.mm' | wc -l`).

- [ ] `components/brave_wallet/browser/cardano/cardano_cip30_serializer.cc:35,56,83,105`
- [ ] `components/brave_wallet/browser/fil_transaction.cc:271`

### gin `SetMethod<&T::M>()` compile-time form

gin's `ObjectTemplateBuilder` now prefers `SetMethod<&T::M>("m")`,
`SetProperty<&T::Get>("p")` and `SetLazyDataProperty<&T::Get>()` for anything
that isn't a bound callback; they skip the per-call callback holder
([76772fc798ced](https://chromium.googlesource.com/chromium/src/+/76772fc798cedbbf37df6c25dc87ff4c63c31cab),
e.g.
[50940ba33265f](https://chromium.googlesource.com/chromium/src/+/50940ba33265fcd8808b860d6f3869b60a052ce2)).
`.SetMethod("x", &…` fell from 314 to 34 in 8104.1..8107.2, and to 31 at 8111.0
through 8115.1
([0ade64b5d6157](https://chromium.googlesource.com/chromium/src/+/0ade64b5d6157e247ff7978a0e570746a88a7aae))
(`git grep -E '\.SetMethod\("[^"]+", *&' <tag> -- '*.cc' | wc -l`).

- [ ] `components/brave_wallet/renderer/js_cardano_provider.cc:76-77`
- [ ] `components/brave_wallet/renderer/js_cardano_wallet_api.cc:76-87`
- [ ] `components/brave_wallet/renderer/js_ethereum_provider.cc:305`, `:403-413`
- [ ] `components/brave_wallet/renderer/js_polkadot_provider.cc:61`
- [ ] `components/brave_wallet/renderer/js_solana_provider.cc:185-197`
- [ ] `components/skus/renderer/skus_js_handler.cc:315-320`
- [ ] `components/speedreader/renderer/speedreader_js_handler.cc:76-77`

### iOS: `TypedProfileKeyedServiceFactoryIOS`

`ProfileKeyedServiceFactoryIOS` now says "Prefer inheriting from
TypedProfileKeyedServiceFactoryIOS instead", and a script is converting upstream
factories
([3cc80ea5bcdc3](https://chromium.googlesource.com/chromium/src/+/3cc80ea5bcdc3)).
Classic factories fell from 234 to 135 and typed ones rose from 7 to 106 between
8107.2 and 8109.1, to 127 and 114 at 8111.0, to 50 and 191 at 8113.1, and to 3
(the factory infrastructure) and 238 at 8115.1
(`git grep -l 'public ProfileKeyedServiceFactoryIOS' <tag> -- ios components`;
the typed count uses `TypedProfileKeyedServiceFactoryIOS<`). The idiom is
`explicit Factory(PassKey key)` calling
`TypedProfileKeyedServiceFactoryIOS(std::move(key), "Name")`, with generated
`GetForProfile()` and `GetInstance()`. Brave has 15 classic factories and no
typed ones, all under `ios/browser/`:

- [ ] `ai_chat/ai_chat_service_factory.h`, `ai_chat/model_service_factory.h`,
      `ai_chat/tab_tracker_service_factory.h`
- [ ] `api/webcompat_reporter/webcompat_reporter_service_factory.h`
- [ ] `brave_account/brave_account_service_factory_ios.h`
- [ ] `brave_ads/ads_service_factory_ios.h`
- [ ] `brave_origin/brave_origin_service_factory.h`
- [ ] `brave_shields/brave_shields_settings_service_factory.h`
- [ ] `brave_wallet/brave_wallet_service_factory.h`
- [ ] `debounce/debounce_service_factory+private.h`
- [ ] `favicon/brave_ios_favicon_loader_factory.h`
- [ ] `misc_metrics/profile_misc_metrics_service_factory.h`
- [ ] `serp_metrics/serp_metrics_service_factory_ios.h`
- [ ] `skus/skus_service_factory.h`
- [ ] `url_sanitizer/url_sanitizer_service_factory+private.h`
- [ ] `favicon/brave_ios_favicon_loader_factory.mm:49-51`: name the testing
      factory as `BraveIOSFaviconLoaderFactory::TestingFactory` with
      `base::BindOnce`, as upstream does ahead of restricting the base headers
      ([c89e7d662d1eb](https://chromium.googlesource.com/chromium/src/+/c89e7d662d1eb0d80fb16e4835b2371171ed3fa5))

### `ui_public_dependencies` → direct deps

Upstream finished removing `browser_public_dependencies` and its `browser/ui`
counterpart, so targets list their direct deps
([1d4b690ab57b](https://chromium.googlesource.com/chromium/src/+/1d4b690ab57b511ef64f9bb681aef7127036573a)).
The rebase routed Brave's UI deps through a new group with the same role
("[cr158] Route the Brave UI public deps through `browser`").

- [ ] `browser/ui/BUILD.gn:39` `ui_public_dependencies` and
      `browser/sources.gni:787` `brave_chrome_browser_public_deps`: move the
      entries to the targets that use them, and verify with `gn check`

## 4. Obsolete Brave code

### Test filters for deleted tests

[064bca2b1e8bc](https://chromium.googlesource.com/chromium/src/+/064bca2b1e8bc9dfdd45d6f6175b67df938e52b7)
deleted `request_storage_access_for_browsertest.cc`, which held the
`RequestStorageAccessFor*` and `TopLevelStorageExemptionReasonMetricTest`
suites.
[22692370687c0](https://chromium.googlesource.com/chromium/src/+/22692370687c0b3d8d76a9cde2c8f7fd309dc9aa)
deleted `PasswordChangePageStabilityWaiterTest`.
[1f3e03da28c3a](https://chromium.googlesource.com/chromium/src/+/1f3e03da28c3a6b766026f70cf55bb3183a77b8e)
deleted `PromoBubbleAccept_OpensDiceSignInTab`,
[6f95a3dcaee63](https://chromium.googlesource.com/chromium/src/+/6f95a3dcaee63a9be2614a52453824294c6e2abb)
`MaterialTabStripColorMixerTest`, and
[c101b64eff937](https://chromium.googlesource.com/chromium/src/+/c101b64eff9374cf0f5fa30a35a818bf55f670ff)
rewrote `BrowserActionRuntimeError` as an MV3 test without the deprecation
warning the filter comment cites.

- [ ] `test/filters/browser_tests.filter:917` and `:1618-1622`
      (`RequestStorageAccessFor*`)
- [ ] `test/filters/browser_tests.filter:2042`
      (`TopLevelStorageExemptionReasonMetricTest`, with its comment)
- [ ] `test/filters/browser_tests.filter:438`
      (`ErrorConsoleBrowserTest.BrowserActionRuntimeError`, now
      `ActionRuntimeError`) and `:1231` (`PromoBubbleAccept_OpensDiceSignInTab`)
- [ ] `test/filters/browser_tests.filter:93`
      (`OmniboxAimPopupDeferCreationPendingBrowserTest.LocationBarActionCreatesAimContent`,
      deleted by
      [8060af6aee203](https://chromium.googlesource.com/chromium/src/+/8060af6aee2032c615a7be3cfb92120b54980a74))
- [ ] `test/filters/unit_tests.filter:200` (`MaterialTabStripColorMixerTest.*`)
- [ ] Generated filters (`test/filters/generated/`): rerun
      `tools/chromium_tests_analysis/update-upstream-flake-filters.py`. Entries
      that should drop out: `GlicMetricsTest.BasicUkm` and `BasicVisible`
      (`unit_tests-linux-ubsan.filter:135,138`,
      [080ca3a5a8d4f](https://chromium.googlesource.com/chromium/src/+/080ca3a5a8d4f43d3eaec7fcc74b376101f98763)),
      `browser_tests-win.filter:4818`, `unit_tests-win-asan.filter:42`, the STTS
      auto-open suites
      ([333a5c08f6c95](https://chromium.googlesource.com/chromium/src/+/333a5c08f6c95030770346e870834f63fb45c830)),
      the Lens test moved to interactive_ui_tests
      ([278b2a7d149cd](https://chromium.googlesource.com/chromium/src/+/278b2a7d149cd21210bdabe3280db32317884e51)),
      `ExactLatency_HighHardwareBufferSize_CapsAtMaxBufferSize`
      ([296a38e06f1e7](https://chromium.googlesource.com/chromium/src/+/296a38e06f1e75eb3b1b36054c185098a3130cde))
      and `HorizontalTabViewTest.HorizontalSeparators_RTL`
      ([22cdddd858c56](https://chromium.googlesource.com/chromium/src/+/22cdddd858c5676db8e2ec079c93897f45911f70)),
      `GlicMetricsTest.BasicStopReasonByUser`
      (`unit_tests-linux-ubsan.filter:132`,
      [d557ccb21387c](https://chromium.googlesource.com/chromium/src/+/d557ccb21387c))
      and `IncognitoApiTest.IncognitoPopup` (`browser_tests-linux.filter:3666`;
      the suite became `All/…`). At 8113.1, also
      `GlicMetricsTest.SessionDuration_LogsError`
      (`unit_tests-linux-ubsan.filter:153`,
      [ce104f637b17](https://chromium.googlesource.com/chromium/src/+/ce104f637b171e4f36d7c65134e50fcdf22064de)),
      and `ExtensionUninstallDialogImplBrowserTest.DialogAnchoredInCorrectPlace`
      (`browser_tests-linux.filter:3531`, `browser_tests-win.filter:3807`),
      which moved to `ExtensionUninstallDialogImplViewsContainerBrowserTest`
      ([5e811ca2ca5d](https://chromium.googlesource.com/chromium/src/+/5e811ca2ca5d68ca4ccd612a65ae5fc584ef3e7d)).
      At 8115.1, also `GlicButtonThemeTest.CustomThemeFallback`
      (`browser_tests-linux.filter:3618`, `browser_tests-win.filter:3939`),
      fixed and re-enabled by
      [dd9b3c1acc142](https://chromium.googlesource.com/chromium/src/+/dd9b3c1acc1423c6d8de2f8adb0fdbb84b1057af)

### Test filters that stopped matching

The Indigo suites became `TEST_P` (`All/…/*`), and their legacy-only cases moved
to new `IndigoLegacyBrowserTest` and `IndigoPageActionControllerLegacyTest`
suites
([8a2bd2103c385](https://chromium.googlesource.com/chromium/src/+/8a2bd2103c385d59c6373503adb3797ddc06dc60)),
so Brave's filters silently re-enable them. Other tests were renamed.

- [x] Indigo `browser_tests` and `unit_tests` filters, `MaxSessionsAndRecency`
      ([b99eae6c5499f](https://chromium.googlesource.com/chromium/src/+/b99eae6c5499f5bb242e62fa93a6ea9a9e66189e))
      and `LoadsAccountAwareDesign`
      ([1f3e03da28c3a](https://chromium.googlesource.com/chromium/src/+/1f3e03da28c3a6b766026f70cf55bb3183a77b8e)):
      fixed by "[cr158] Filter upstream tests as needed"
- [ ] `test/filters/browser_tests.filter:3088,3090`: the old
      `IndigoHighDsfBrowserTest.*` and `IndigoOnboardingBrowserTest.*` entries
      match nothing now that `:743-744` cover the `All/…` suites; drop them
- [ ] `test/filters/browser_tests.filter:1745`:
      `ExtensionOpenSidePanelViewsToolbarBrowserTest.PinExtensionViaSidePanelJourneyCompletion`
      ([301a9d992e895](https://chromium.googlesource.com/chromium/src/+/301a9d992e895117f69cb54a48ff21a6787f2cc3))
- [ ] `test/filters/browser_tests.filter:1963`:
      `TtcSessionJournalImplBrowserTest.JournalFollowsTheSessionTask`
      ([70ef0900441d9](https://chromium.googlesource.com/chromium/src/+/70ef0900441d902273eb3326fe9a3e861fdc4697))
- [ ] `test/filters/browser_tests.filter:1396`:
      `SendTabToSelfBubbleControllerBrowserTest.ScrollPositionPropagated_ScrolledPage`
      ([65d99b0dbd18e](https://chromium.googlesource.com/chromium/src/+/65d99b0dbd18ef70fc023a727bae6f7a4afcd4b9))
- [ ] `test/filters/components_unittests.filter:282-284`:
      `GetPreferredAccountForPromo*` → `GetPreferredAccountForSignin*`
      ([69060d1c866bc](https://chromium.googlesource.com/chromium/src/+/69060d1c866bca1bcd1784e0b9c1bf43626f94c1))

### Dead files and stale references

- [ ] `docs/best-practices/plaster.md:153`: replace the
      `browser_public_dependencies` example, which no longer exists upstream
- [ ] `chromium_src/chrome/browser/ui/views/side_panel/DEPS`: none of its rules
      is used since the rebase deleted `side_panel_helper.cc`
      ([9f65ed5be8044](https://chromium.googlesource.com/chromium/src/+/9f65ed5be804419c90e5bcfe68ffd5c941140b80))
- [ ] Pre-existing: `components/network_time/network_time_tracker_unittest.cc`
      is in no GN target and includes headers deleted long ago
      (`client_update_protocol/ecdsa.h`,
      [e4bbdd3a03a5e](https://chromium.googlesource.com/chromium/src/+/e4bbdd3a03a5e6c5c3372e31adde5485fc1bee96));
      delete it
- [ ] `browser/ephemeral_storage/browsing_history_cleaner.cc:30`: the comment
      names `GetAllSyncedClientsGroupedByName`, now
      `GetSyncedDevicesForFiltering`
      ([dcad9dc93cef3](https://chromium.googlesource.com/chromium/src/+/dcad9dc93cef3785818cbc7e38d4bbc56f8e7ca8))

- [ ] `browser/resources/settings/br/autofill_section.ts`: pre-existing, from
      "[cr153][Settings] Fixes autofill page". It targets
      `settings-autofill-section`, a tag that no longer exists upstream, and
      `contact_info_page.html.ts.lit_mangler.ts` already hides
      `#emailSharedMenu`. Delete it and its import (`br/index.ts:16`)
- [ ] `rewrite/chrome/test/BUILD.gn.yaml`: upstream locked
      `//chrome/test:test_support` down to `sources = []` ("Do not add new
      sources",
      [4d744283243d4](https://chromium.googlesource.com/chromium/src/+/4d744283243d4),
      [9f5bf17f489fb](https://chromium.googlesource.com/chromium/src/+/9f5bf17f489fb)),
      but the "Add the Brave test support sources" substitution still appends
      `testing_brave_browser_process.{cc,h}` to it, and
      `//brave/test:test_support` (`test/BUILD.gn:55`) already compiles them.
      Drop that substitution (unverified: `gn check` on the header without the
      circular-include allowance)
- [ ] `browser/resources/settings/br/passwords_section.ts`: pre-existing since
      M117. Settings has no `passwords-section` since
      [f33cf9c85867d](https://chromium.googlesource.com/chromium/src/+/f33cf9c85867d7aaa36b91f3e869d64a6a13afd0),
      and no upstream template has `#checkPasswordsLinkRow`. Delete it, its
      import (`br/index.ts:27`) and `browser/resources/settings/sources.gni:93`
- [ ] `android/java/apk_for_test.flags`: pre-existing keep rules for members
      that no longer exist: `:165`
      `ToolbarManager.mOverlayPanelVisibilitySupplier`, `:168`
      `ToolbarManager.mTabGroupUi`, `:229` `BookmarkUtils.isSpecialFolder` and
      `:628` `LocationBarMediator.onAssistantVoiceSearchServiceChanged`
- [ ] "[WIP][cr158][Android] Drop Nala icons orphaned by the main settings
      cleanup": decide whether `ic_credit_card` and `ic_location_on` belong on
      upstream's `AutofillAndPasswordsFragment` rows, then drop the WIP tag

## 5. Optional

### `kUniversalOptOut` is on by default

Upstream enabled `kUniversalOptOut` and set `target_locations` to `us-ca`
([66049b03adbb3](https://chromium.googlesource.com/chromium/src/+/66049b03adbb3),
reverted and relanded as
[437e79d117f52](https://chromium.googlesource.com/chromium/src/+/437e79d117f52cebbfec130e51b3d84b67bbf085)).
`UniversalOptOutServiceFactory` now creates the service in Brave, which records
location history from the variations country and account capabilities. The UI
stays hidden because `kUniversalOptOutSettings` is still off, and Brave has its
own Sec-GPC. Brave pins nothing here: `ReadIsSubjectToUniversalOptOutCapability`
(`base/compile_overridden_features.inc:153`) isn't an upstream feature at either
tag. On iOS 18.4 to 26, `IsUniversalOptOutEnabled()` also needs
`kUniversalOptOutExtension`, which is now on too
([72587259dd464](https://chromium.googlesource.com/chromium/src/+/72587259dd46458a036f9f20a61707a3137020eb)),
so it's on for every iOS version from 18.4; the built-in extensions now also run
in incognito tabs
([6da1866e890de](https://chromium.googlesource.com/chromium/src/+/6da1866e890deb5191b0fd17c9e14c03ecb684a4)).

- [ ] Pin `universal_optout::features::kUniversalOptOut` and
      `kUniversalOptOutExtension` (`components/universal_optout/features.cc:24`)
      disabled with a `set_feature_flag_default_state` plaster, and add them to
      `app/feature_defaults_unittest.cc`

### Unpinned flags that switch to parallel implementations

Both are off by default. `kWebUIOmniboxFullPopup` (already in M157's "Flag-gated
replacement UIs") got about nine more commits in this range (e.g.
[eb3e93b1fa699](https://chromium.googlesource.com/chromium/src/+/eb3e93b1fa6998c7a6e81ab2af2a86c12318781e)).
`syncer::kCrossDeviceTabsInAppMenu` moves other-device tabs out of the History
submenu into a new app menu entry
([b99eae6c5499f](https://chromium.googlesource.com/chromium/src/+/b99eae6c5499f5bb242e62fa93a6ea9a9e66189e)),
skipping Brave's `RecentTabsSubMenuModel` plasters.

- [ ] Pin `kCrossDeviceTabsInAppMenu` disabled, or port the customizations

`ntp_features::kNtpComponentExtension` (off) loads a component extension whose
manifest overrides `chrome://newtab`
([d58babee2acb6](https://chromium.googlesource.com/chromium/src/+/d58babee2acb62c4e9323743b929b05e74921f7f)).
`BraveComponentLoader` calls through to upstream's loader and the extension is
allowlisted, so it would replace Brave's NTP if the flag flips.

- [ ] `rewrite/components/search/ntp_features.cc.yaml`: pin
      `kNtpComponentExtension` disabled, and add it to
      `app/feature_defaults_unittest.cc`; optionally deny
      `extension_misc::kNewTabPageExtensionId` in
      `chromium_src/chrome/browser/extensions/component_extensions_allowlist/allowlist.cc:34`

Brave pins `kContextualTasks` and `kContextualTasksCookiePrefetch` disabled, but
two newer flags switch to a rearchitected implementation without checking them.
`IsContextualTasksSidePanelRearchitectureEnabled()` doesn't read
`kContextualTasks`, and `kContextualTasksSidePanelRearchitecture` adds an "AI
Mode" submenu to the tab menu
([87f2181418af6](https://chromium.googlesource.com/chromium/src/+/87f2181418af6)).
`kContextualTasksRearchitecture` makes `TabFeatures::Init` create
`ContextualTasksTabNavigationObserver`, and since 8115.1
`IsContextualTasksUIEnabled()` is also true under
`kContextualTasksSidePanelRearchitecture`
([f7e1f219a6e62](https://chromium.googlesource.com/chromium/src/+/f7e1f219a6e62)).
Both are off today.

- [ ] Pin `kContextualTasksSidePanelRearchitecture` and
      `kContextualTasksRearchitecture` disabled

### Windows 11: modal default-browser prompt and OS notification

[b6c9c5f69f4c](https://chromium.googlesource.com/chromium/src/+/b6c9c5f69f4cffccd567ccf6e1ca084c02669540),
reverted and relanded as
[d9a532252185d](https://chromium.googlesource.com/chromium/src/+/d9a532252185d1215efc1b5089b3634e85d5601f),
enables `kDefaultBrowserPromptSurfaces` (now a modal dialog with an
illustration), `kDefaultBrowserChangedOsNotification` (a toast when another
browser becomes the default) and `kPerformDefaultBrowserCheckValidations`. The
reland enables them on every platform; their helpers check for Windows 11. Brave
shows upstream's startup prompt and pins none of them.

- [ ] Decide whether Brave wants the modal and the toast; if not, pin the first
      two disabled in
      `chrome/browser/default_browser/default_browser_features.cc` and add them
      to `app/feature_defaults_unittest.cc`

### Linux: the GTK accent tints Brave's palette

With the GTK or QT theme, `ThemeService::GetColorProviderKey()` now keeps the
accent as `user_color_source` before it checks `GetIsGrayscale()`
([b43d41c50b3e](https://chromium.googlesource.com/chromium/src/+/b43d41c50b3eee4b059b0d52132a1830dd79962b)).
`BraveThemeService::GetIsGrayscale()` is no longer reached there, so
`ShouldUseAccentTintedPalette()` (`ui/color/brave_ref_color_mixer.cc:247`) and
the tab and omnibox tints follow the GTK accent. Only users who chose GTK/QT are
affected.

- [ ] Decide whether to accept it; if not, override the key in
      `browser/themes/brave_theme_service.cc`, with a Linux case in
      `brave_theme_service_browsertest.cc`

### Mac: dock icon plaster may duplicate upstream's fix

Upstream now redraws the dock tile when the icon appearance changes
([71a792a0a056e](https://chromium.googlesource.com/chromium/src/+/71a792a0a056e0f66c89fd16476f9cf8db29b47e)).
Brave's plaster (master 35ac91eba0c) fixed the same symptom by dropping the
`DockTileView` and its progress ring, on the premise that a redraw only gets a
flattened icon.

- [ ] On macOS, check whether upstream's redraw follows light, dark and tinted
      modes; if so, drop `rewrite/chrome/browser/ui/cocoa/dock_icon.mm.yaml`,
      `chromium_src/chrome/browser/ui/cocoa/dock_icon.mm` and
      `patches/chrome-browser-ui-cocoa-dock_icon.mm.patch`

### Android: "On startup" settings row

`kOnStartupSettingsUi` (off) adds an `on_startup` row to the main settings on
desktop-class devices
([8af360c5308aa](https://chromium.googlesource.com/chromium/src/+/8af360c5308aa)).
`BraveMainPreferencesBase` removes upstream rows by key and has no entry for it.

- [ ] Remove `on_startup` there before the flag launches

### Linux: `kLinuxPerProfileAppId`

The flag is off by default. It adds per-profile `.desktop` entries and a
per-profile `WM_CLASS`
([2237b5ee29980](https://chromium.googlesource.com/chromium/src/+/2237b5ee29980),
[bb9e4e4cacce1](https://chromium.googlesource.com/chromium/src/+/bb9e4e4cacce1)).
Brave's `shell_integration_linux.cc` patch only changes `GetIconName()`.

- [ ] Before it launches, check the packaged desktop file name and icon

### Pre-existing: Brave's empty temporary-grant list misses `ShouldTypeExpireActively()`

Brave renames `GetTypesWithTemporaryGrants{,InHcsm}` to `_ChromiumImpl` and
returns empty lists, but upstream's in-file `ShouldTypeExpireActively()`
([41742ec67e28c](https://chromium.googlesource.com/chromium/src/+/41742ec67e28cb4d84cfde405a350fb87bfdcc12))
calls the renamed upstream list. `kActiveContentSettingExpiry` is now on
([ad6c799cdc686](https://chromium.googlesource.com/chromium/src/+/ad6c799cdc686d2e157eaca475a0581848d9c813)),
so those types skip expired-rule filtering. No Brave writer of expiring grants
for them was found.

- [ ] `chromium_src/components/content_settings/core/browser/content_settings_utils.cc:15-21,51-58`:
      make the in-file callers see Brave's list (e.g. `preempt_function_impl`)

### Send tab to self: `kOfferReauth` is no longer gated

[7422f4fa4b043](https://chromium.googlesource.com/chromium/src/+/7422f4fa4b043b6988b576fe7b26bd185386836c)
removed the `kSendTabToSelfEnhancedDesktopUI` gate on `ShouldOfferReauth`. Brave
filters `kOfferSignIn` and `kInformNoTargetDevice` but not `kOfferReauth`; it's
unreachable only because sign-in is disallowed.

- [ ] `chromium_src/components/send_tab_to_self/entry_point_display_reason.cc:32-33`

### Android: send-tab-to-self sheet is 8dp short in phone landscape

`getNonListHeightForState()` now uses
`getBottomActionsPaddingBottomPx(showManageDevices)`
([dc1ef6cb542b1](https://chromium.googlesource.com/chromium/src/+/dc1ef6cb542b15f13501b58a728e4163bea2bd8c)),
but Brave hides the footer, so the real padding is 16dp, not 8dp.

- [ ] `rewrite/chrome/browser/share/android/java/src/org/chromium/chrome/browser/share/send_tab_to_self/EnhancedTargetDevicePickerView.java.yaml`:
      force `showManageDevices` false there too

### Android: Brave's desktop accelerators now reach `IsChromeAccelerator()`

Under `kUniversalKeyboardHandling` (enabled), Android reads
`GetAcceleratorList()`
([1a2099d15b95e](https://chromium.googlesource.com/chromium/src/+/1a2099d15b95e5cd2973bf71def913be400c245d),
[304c9d092f12d](https://chromium.googlesource.com/chromium/src/+/304c9d092f12df112655fe1f74b1c9cc1e129d3c)).
Brave appends desktop-only commands there; the extension command conflict check
sees them, and since
[42c5037e33720](https://chromium.googlesource.com/chromium/src/+/42c5037e337209fdbc43ff003d6e8b08c8bd20dc)
so do the app menu's shortcut hints (`GetAcceleratorForCommandId()`, e.g.
`IDC_TOGGLE_TAB_MUTE`, `IDC_NEW_SPLIT_TAB`).

- [ ] `chromium_src/chrome/browser/ui/accelerator_table.cc:16-41`: exclude
      `kBraveAcceleratorMap` on Android, or keep only what Android implements

### Android: Ads and Rewards databases trim memory when pressure subsides

Android's `MemoryPressureMonitor` now sends `NONE` when pressure subsides
([f9f29f451c72a](https://chromium.googlesource.com/chromium/src/+/f9f29f451c72a3e505db1553edeacba397ff7658)),
and Brave's listeners ignore the level.

- [ ] `components/brave_ads/core/internal/database/database.cc:461` and
      `components/brave_rewards/core/engine/rewards_database.cc:345`: return
      early on `NONE`, or move to `base::MemoryConsumer`

### Pre-existing: Brave settings subpages don't update the tab title

`<settings-subpage>` sets `document.title` only while its `route-path` matches
the current route (`settings_subpage.ts:175`), and the router sets no title for
subpage routes without one, which all of Brave's `createChild()` routes in
`browser/resources/settings/brave_routes.ts` are. Upstream added `route-path` to
the last of its 80 subpages
([d491b2f13feb6](https://chromium.googlesource.com/chromium/src/+/d491b2f13feb693318e46dd6574be05a6b6ba11e));
none of Brave's 14 has it, so the tab keeps the previous page's title.

- [ ] Add `route-path$="[[routePath]]"` to the `<settings-subpage>` in
      `browser/resources/settings/`
      `site_settings/site_settings_{autoplay,brave_ai,cardano,ethereum,google,shields,solana}.html:6`,
      `brave_default_extensions_page/brave_extensions_manifest_v2_subpage.html:47`,
      `brave_leo_assistant_page/{customization_subpage.html:59,model_config_subpage.html:9}`,
      `brave_sync_page/brave_sync_subpage.html:29`,
      `brave_wallet_page/wallet_networks_subpage.html:4`,
      `default_brave_shields_page/brave_adblock_subpage.html:179` and
      `email_aliases_page/email_aliases_page.html:1`
- [ ] Pass `route-path` from the views that host them where it's missing:
      `brave_wallet_page/wallet_page_index.html:23`,
      `brave_sync_page/brave_sync_page_index.html:15`, the Leo, Shields and
      extensions indexes, and `settings-email-aliases-page` in
      `chromium_src/chrome/browser/resources/settings/autofill_page/autofill_page_index.html.ts.lit_mangler.ts:16`

### WebGL on WebGPU skips Brave's WebGL farbling

`WebGLRenderingContextWebGPUBase` derives from neither WebGL base class and has
its own `getParameter()`, `getSupportedExtensions()`,
`getShaderPrecisionFormat()` and `readPixels()`, which this range filled in
(e.g.
[74d2f88ce3a12](https://chromium.googlesource.com/chromium/src/+/74d2f88ce3a12),
[86d739b6ae199](https://chromium.googlesource.com/chromium/src/+/86d739b6ae199)).
Brave only hooks `webgl_rendering_context_base.cc` and
`webgl2_rendering_context_base.cc`. `WebGLOnWebGPU` has no `status:`, so only
`--enable-blink-features` reaches it.

- [ ] Before it ships, farble the WebGPU-backed contexts, or
      `EnableFeatureFromString("WebGLOnWebGPU", false)` in
      `renderer/brave_content_renderer_client.cc`

### Script the Android names-in-strings check

Resolving every `BraveReflectionUtil` literal, `BytecodeTest` string and
bytecode adapter target (with parameter lists) against the upstream class at the
new tag found items in M157, 8104.1 and 8107.2.

- [ ] Add a `tools/cr/` script that does this for two tags

### License metadata uses SPDX identifiers

Upstream normalised `SPECIAL_CASES` licenses
([ebe50178d4754](https://chromium.googlesource.com/chromium/src/+/ebe50178d4754320c29492023f994de49dcae68b)).

- [ ] `script/brave_license_helper.py:319`: `"BSD"` → `"BSD-3-Clause"`

### Android: "Report an issue" in the Help submenu

Upstream re-added "Report an issue" under `help_parent_menu_id`
([cd627cf679c4e](https://chromium.googlesource.com/chromium/src/+/cd627cf679c4e1089c9712c53457b6d3f5737f8d)).
Brave removes only `help_id`, and doesn't pin `kSubmenusInAppMenu` or
`kSubmenusInAppMenuLff`, both off by default. Predates the range.

- [ ] `android/java/org/chromium/chrome/browser/tabbed_mode/BraveTabbedAppMenuPropertiesDelegate.java:1010`:
      also handle the submenu item, or pin both flags off

### Android: `MultiWindowUtils` redirect now catches a private overload

[c934e7cc992b7](https://chromium.googlesource.com/chromium/src/+/c934e7cc992b7)
added a private static `isLinkNavigationToNewWindowSupported(Set<Integer>)` that
the public no-arg method and `isLinkNavigationToIncognitoWindowSupported()`
call. `changeMethodOwner` matches by name, so those calls are redirected to
`BraveMultiWindowUtils`, which has no `(Set)` variant; resolution falls back to
the private upstream method, which works only by luck.

- [ ] `build/android/bytecode/java/org/brave/bytecode/BraveMultiWindowUtilsClassAdapter.java:32`
      and
      `android/javatests/org/chromium/chrome/browser/BytecodeTest.java:980-985`:
      redirect only the descriptor Brave overrides

### Android: upstream's Play in-app update flow

[f742d8b3cd709](https://chromium.googlesource.com/chromium/src/+/f742d8b3cd709)
adds `InAppUpdateController` behind `kInAppUpdateFlow` (off). With the flag off,
the app menu "Update" item keeps its old intent fallback.

- [ ] `android/java/org/chromium/chrome/browser/app/BraveActivity.java:443-460,1711-1790`:
      if the flag launches, adopt upstream's flow instead of Brave's own

### Android: dead null check in `BraveExternalNavigationHandler`

[543e673b21d3](https://chromium.googlesource.com/chromium/src/+/543e673b21d3d65611e526f308562626d08aa824)
made the target URL argument non-null.

- [ ] `android/java/org/chromium/chrome/browser/externalnav/BraveExternalNavigationHandler.java:56`:
      drop `intentTargetUrl != null`

## For privacy review

- WebHID is on by default on Android
  ([3faefe341d971](https://chromium.googlesource.com/chromium/src/+/3faefe341d971a2abb4d028adea297ec50b489a1));
  Brave Android has no HID handling.
- `kPervasiveSharedDictionaries` is on by default, with an unpartitioned
  dictionary store
  ([f5be8340b1c35](https://chromium.googlesource.com/chromium/src/+/f5be8340b1c35c35005389aa4239e5df47dbee23)).
- `kEnterpriseTabTitleReporting` is on by default on desktop
  ([b3d46186b7c41](https://chromium.googlesource.com/chromium/src/+/b3d46186b7c414eb5a90399876867fb68bff1c4d)).
- The Text Detection API has a browser OCR backend
  ([bf3e92232a466](https://chromium.googlesource.com/chromium/src/+/bf3e92232a466fa3db915c85017f0ba8456a94c7));
  `TextDetector` is still experimental.
- Sessions sync always uses the preferred device display name now that
  `kSyncSessionsUsePreferredDisplayName` is gone
  ([930258c156df9](https://chromium.googlesource.com/chromium/src/+/930258c156df9cfc0f7af0c7e7c98fc7f147b5a8)).
- `kSmartRestartLockScreen` is on by default: the browser restarts on the lock
  screen to apply updates; Brave blocks it only under Sparkle
  ([c9f6a2a0b473d](https://chromium.googlesource.com/chromium/src/+/c9f6a2a0b473dc8f35a34f59558d582a4482c738)).
- Reporting API crash reports now cover same-process subframes
  ([94f92bd6bd6ce](https://chromium.googlesource.com/chromium/src/+/94f92bd6bd6ce5eb7037225360a5b882cfeab703)).
- `kFetchErrorCategoryLogging` is on: UKM for `fetch()` errors
  ([c8571f70483d9](https://chromium.googlesource.com/chromium/src/+/c8571f70483d9bed0271d14960955111ac048ab2)).
- The EyeDropper API shipped on Android
  ([c374476136290](https://chromium.googlesource.com/chromium/src/+/c37447613629055fffb8828e3e1f27c22e28e733)).
- "Ask Google about this page" Lens IPH registered, behind
  `kWebUIOmniboxAskGAboutThisPage`
  ([548fe68d2d70a](https://chromium.googlesource.com/chromium/src/+/548fe68d2d70a9cd36cd58d3848ef7a3b6970bf6)).
- Android promos registered: search engine choice, fullscreen sign-in,
  notification permission, tips opt-in
  ([e611fd74e5237](https://chromium.googlesource.com/chromium/src/+/e611fd74e5237a48275bb470cf8b099856de5e02)).
- Always-on-top popups prototype, behind a disabled Blink feature
  ([afff432d76ede](https://chromium.googlesource.com/chromium/src/+/afff432d76ede098d6638042dacdd39e71c47e19)).
- History can be filtered by synced device
  ([d6b17880fcaf9](https://chromium.googlesource.com/chromium/src/+/d6b17880fcaf94dc6536e01ec62f6aa0c47264ef)).
- Universal opt-out location history (see "`kUniversalOptOut` is on by
  default").
- Contextual tasks: `kContextualTasksSidePanelTabState` and
  `kContextualTasksTabBasedSuggestions`, plus the tab-menu "AI Mode" submenu.
- `kHappinessTrackingSurveysForDesktopScheduledRestart` and the OTP-dismissal
  HaTS survey in `AutofillExternalDelegate::OnSuggestionsHidden`
  ([4af54e9ea1b7](https://chromium.googlesource.com/chromium/src/+/4af54e9ea1b7)).
- `kClientSideDetectionServerModelMultimodal`.
- Private AI TLS certificate downloader, fetching from Google's static content
  service, with no consumer yet
  ([89c7d49fbb0a3](https://chromium.googlesource.com/chromium/src/+/89c7d49fbb0a3)).
- Cross-Origin Storage: a per-`BrowserContext` store, deliberately not
  partitioned by `StorageKey`, so containers and ephemeral storage would share
  it; not bound to the renderer yet, and `CrossOriginStorage` has no `status:`
  ([9bc8886a94f40](https://chromium.googlesource.com/chromium/src/+/9bc8886a94f40431827dfaa8a11b2eb38ab5de84)).
  Consider `EnableFeatureFromString("CrossOriginStorage", false)` in
  `renderer/brave_content_renderer_client.cc`.
- Permission prediction models delivered through the component updater (see "The
  geolocation permission prediction model is fetched through the component
  updater").
- `kNtpComponentExtension` (off) binds the NTP and realbox handlers and
  segmentation in an extension page.
- `kLensSelectionSuggestions` (off): `LensSuggestionTool` in `TabFeatures` opens
  the Lens overlay with a cropped screenshot
  ([2891aa8c9d6d9](https://chromium.googlesource.com/chromium/src/+/2891aa8c9d6d9)).
- Android: TTC `kSwitchTab`, `kCloseTab` and `kOpenKnownPage` enabled
  ([cc2633da88c5](https://chromium.googlesource.com/chromium/src/+/cc2633da88c5),
  [22b0443a3351](https://chromium.googlesource.com/chromium/src/+/22b0443a3351),
  [0d1306278ca4a](https://chromium.googlesource.com/chromium/src/+/0d1306278ca4a223587215210ec028db6ee1b8cb));
  still gated by Brave's `Ttc` pin.
- Android: `kSecondaryGoogleAccountsSigninAllowedOnAndroid` (off), and the
  "Aurora" AI Mode tile in Most Visited
  ([ce7727efc0c6c](https://chromium.googlesource.com/chromium/src/+/ce7727efc0c6c)).
- Gmail OTP opt-in bubble as an omnibox page action, `kGmailOtpOptIn`
  ([55b85589c576](https://chromium.googlesource.com/chromium/src/+/55b85589c576afe5c515265bee9ac18dc9e3e42c));
  needs a Google account.
- `kAndroidDesktopUAModel` (off) would send the device model in
  `Sec-CH-UA-Model`
  ([482f8fbb9334](https://chromium.googlesource.com/chromium/src/+/482f8fbb933447204c81d26a00614e4b0e4a602d));
  Brave's `user_agent_utils.cc` override still blanks it.
- Android: `kAim3pEntrypoint` is on, an AI Mode NTP entry point for any search
  engine with a config
  ([8a63145f751d](https://chromium.googlesource.com/chromium/src/+/8a63145f751d9e110202d922dee4d44f249e7acb));
  still gated by Brave's `kAimEnabled` pin.
- Safe Browsing temporary "No Protection" mode with six synced prefs, behind
  `kSafeBrowsingNoProtectionTemporary` (off)
  ([afd4d200ae12](https://chromium.googlesource.com/chromium/src/+/afd4d200ae12a2c731d6265bd62971ee57bb31c0)).
- `kLensSendDriveTabUrlsInComposeboxes` (off) sends Drive tab URLs to Lens.
- `kSpellcheckSeparateLocalAndAccountDictionaries` is on: words synced from
  other devices stay in memory only
  ([7ac98441fb0ba](https://chromium.googlesource.com/chromium/src/+/7ac98441fb0ba9c2d6b66ab5e53e1335e22d43b4)).
- The Reading mode omnibox chip records `Accessibility.ReadAnything.*` UKM (see
  "Reading mode chip shows in the omnibox next to Speedreader").
- iOS: `kGeminiQuizzes`, `kGeminiFRERefactor` and `kGeminiFREExperiment` are
  enabled.
- `window.open()` `split`, `splitposition` and `splitsize` features, behind the
  `WindowOpenSplitTab` runtime feature (`status: "test"`)
  ([fbdad5b232bd5](https://chromium.googlesource.com/chromium/src/+/fbdad5b232bd5e7457d2ea20c47283709dda52f7)).
- Android: the Glic `EXTERNAL_TRIGGERING` intent filter ships on all channels,
  behind `kGlicBackgroundTriggering` (off); Brave's `GlicEnabling.java` plaster
  keeps Glic off
  ([50c1ee33a159d](https://chromium.googlesource.com/chromium/src/+/50c1ee33a159d2926be88ac60b1ecdf95bd1fcf3)).
