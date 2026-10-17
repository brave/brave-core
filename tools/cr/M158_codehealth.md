# M158 code health follow-ups

Tasks found by reviewing upstream changes in `157.0.8103.3..158.0.8109.1` (3252
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
- [ ] `browser/brave_tab_helpers.cc:121` `AttachTabHelpers()`: move the helpers
      into `BraveTabFeatures::Init` (`browser/ui/tabs/brave_tab_features.cc`),
      following the per-helper list and idiom in M157's "TabHelpers →
      TabFeatures"; the call from `Init` is only a stopgap
- [ ] Android: same, in `browser/android/brave_tab_features.cc:24`, including
      Ads and Rewards
- [ ] `browser/ui/tabs/brave_tab_features.cc:94-125`: the other helpers follow
      discards through `OnWillDiscardContents`, but `ads_tab_helper_` and
      `rewards_tab_helper_` stay bound to the old contents; make them follow
      discards (`ContentsObservingTabFeature`)
- [ ] `browser/brave_tab_helpers.cc:219` `AttachPrivacySensitiveTabHelpers()`:
      decide which non-tab WebContents that upstream used to cover still need it
      (prerender, no-state prefetch, extension host, thin webview, overlay
      panel). Only AI Chat calls it now. M157 skipped moving
      `BraveShieldsWebContentsObserver` and `EphemeralStorageTabHelper` for this
      reason; tabs need them too now
- [ ] Add a browser test that a new tab has `BraveShieldsWebContentsObserver`
      and `EphemeralStorageTabHelper`, on desktop and Android
- [ ] Stale references: `browser/PRESUBMIT.py:27,99`,
      `docs/best-practices/architecture.md:1875-1902` (ARCH-074),
      `browser/ui/tabs/brave_tab_features.cc:98` and
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

- [ ] Replace `br/settings_main.ts` with
      `chromium_src/chrome/browser/resources/settings/settings_main/settings_main.html.ts.lit_mangler.ts`,
      as `settings_menu.html.lit_mangler.ts` does; drop the import at
      `br/index.ts:34`
- [ ] Check that `pageVisibility_` still carries Brave's keys
      (`br/page_visibility.ts`) in the Lit `pageVisibility_?.X` access
- [ ] `br/settings_ui.ts:156`: check that `this.$.main` still works as the
      scroll target
- [ ] Re-check the Brave wrapper in
      `chromium_src/chrome/test/data/webui/settings/settings_main_test.ts`
      against the rewritten upstream test

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

- [ ] Rewrite the override for the typed template; keep
      `BuildServiceInstanceFor` returning null and `RegisterProfilePrefs` empty
- [ ] `chromium_src/ios/chrome/browser/signin/model/about_signin_internals_factory.cc`
      shadows another factory the same way; check it when upstream converts it
      (about 135 factories remain)

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

- [ ] `chromium_src/components/update_client/request_sender.cc:15-26`: switch to
      a Brave ML-DSA-44 key (needs update server support), or stop building a
      signer from the ECDSA key
- [ ] `rewrite/components/update_client/request_sender.cc.yaml`: update the
      description, which still describes upstream's ECDSA/ML-DSA flag
- [ ] `chromium_src/components/update_client/request_sender_unittest.cc`: the
      `cup2key=1:` expectations and `BraveKeyIgnoresPqcCupSigningFlag` refer to
      the old format and the deleted flag
- [ ] `test/filters/components_unittests.filter:262-266`: re-evaluate
      `-All/RequestSenderTest.CupKeySelection/*` and its comment

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
- [ ] `renderer/brave_content_renderer_client.cc:145`: disable them with
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

### Android: `UndoBarController.showUndoBar(List, List, boolean)` removed

The method became the private `showUndoBar(UndoClosureSnapshot)`, and
`UndoActionData` was replaced by `UndoBarContent`
([7cc7f72c6058d](https://chromium.googlesource.com/chromium/src/+/7cc7f72c6058d93458399b4406a3a7a75dc9b9e1)).
`BraveUndoBarControllerBase` no longer compiles, its reflection call would
return null (no undo bar), and `changeMethodOwner` matches by name only, so the
new call gets redirected to a descriptor Brave lacks.

- [ ] `android/java/org/chromium/chrome/browser/undo_tab_close_snackbar/BraveUndoBarControllerBase.java:14-42`:
      take an `UndoClosureSnapshot`; when undo is off, call `onDismissNoAction`
      with an `UndoBarContent`
- [ ] `build/android/bytecode/java/org/brave/bytecode/BraveUndoBarControllerBaseClassAdapter.java:21`:
      check the redirect against the new descriptor
- [ ] `android/javatests/org/chromium/chrome/browser/BytecodeTest.java:1013-1020`:
      update the `methodExists` signature

### Android: `AppMenu` constructor lost its `boolean` parameter

[a261b745aaa91](https://chromium.googlesource.com/chromium/src/+/a261b745aaa912b3ee4caa60d75adcb3fab8ab55)
removed `disableVerticalScrollbar` and
`AppMenuDelegate.shouldDisableVerticalScrollbar()`.

- [ ] `browser/ui/android/appmenu/internal/java/src/org/chromium/chrome/browser/ui/appmenu/BraveAppMenuDummySuper.java`
      and `BraveAppMenu.java:26-32`: drop the parameter
- [ ] `android/javatests/org/chromium/chrome/browser/BytecodeTest.java:2218-2225`:
      drop `boolean.class` from `constructorsMatch`

### Android: `TabSwitcherActionMenuCoordinator` takes a `TabGroupExistenceChecker`

The constructor and `createOnLongClickListener` take a
`TabGroupExistenceChecker` instead of a `TabWindowManager`; `ToolbarManager`
passes a lambda over `GroupWindowChecker.hasOtherGroups()`
([bdc2956b708e9](https://chromium.googlesource.com/chromium/src/+/bdc2956b708e9345e4cb51b9ca4af9d6cd2a5db2)).

- [ ] `browser/ui/android/toolbar/java/src/org/chromium/chrome/browser/toolbar/top/BraveTabSwitcherActionMenuCoordinator.java:28-32`
- [ ] `browser/ui/android/toolbar/java/src/org/chromium/chrome/browser/toolbar/top/BottomTabSwitcherActionMenuCoordinator.java:22-46`
- [ ] `android/java/org/chromium/chrome/browser/toolbar/BraveToolbarManager.java:450-454`:
      pass the checker upstream's `ToolbarManager` builds
- [ ] `android/junit/src/org/chromium/chrome/browser/toolbar/top/BraveTabSwitcherActionMenuCoordinatorTest.java:34,56,83`

### Android: `kPopularSitesJsonPref` moved to a private header

The popular sites prefs moved to
`components/ntp_tiles/popular_sites_pref_names.h`, whose target is visible only
inside `//components/ntp_tiles`
([6c240d5b67c85](https://chromium.googlesource.com/chromium/src/+/6c240d5b67c857fdd2239333b74a2701fdbcf461)).
Brave's override is likely redundant:
`chromium_src/components/ntp_tiles/most_visited_sites.cc` already forces
`popular_sites_` to null.

- [ ] `browser/brave_profile_prefs.cc:135,190-191`: drop the
      `kPopularSitesJsonPref` default and the include

### Android: per-site Autoplay and Google Sign-In rows aren't set up

The private
`SingleWebsiteSettings.setupContentSettingsPreference(Preference, Integer, boolean, boolean)`
gained a `@Nullable PermissionSetting` parameter
([0c45e73d2543d](https://chromium.googlesource.com/chromium/src/+/0c45e73d2543de1db5f266a76d79b33753a9be02)).
Brave calls the old signature by reflection, so in release builds
`BraveReflectionUtil.invokeMethod` returns null and the rows stay empty.

- [ ] `components/browser_ui/site_settings/android/java/src/org/chromium/components/browser_ui/site_settings/BraveSingleWebsiteSettings.java:77,107`:
      pass `PermissionSetting.class, null` after the `Integer` argument
- [ ] `android/javatests/org/chromium/chrome/browser/BytecodeTest.java:1339`:
      add `PermissionSetting.class` to the `methodExists` signature

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
statics fell from 54 to 20 in 8104.1..8107.2 (e.g.
[49111387fe95e](https://chromium.googlesource.com/chromium/src/+/49111387fe95eb6cefd2465f4b93b74f021b2c39)).
Brave redirects the static `getToolbarSidePaddingForNtp(Context)` with
`changeMethodOwner`, which matches by name only, and an instance method of the
same name already exists. If upstream's caller switches to it, the redirect
targets a method Brave lacks; if the static goes, the pin is lost silently.

- [ ] `build/android/bytecode/java/org/brave/bytecode/BraveOmniboxResourceProviderClassAdapter.java:19-22`
      and
      `android/java/org/chromium/chrome/browser/omnibox/styles/BraveOmniboxResourceProvider.java`:
      move the override to the instance path
- [ ] `android/javatests/org/chromium/chrome/browser/BytecodeTest.java:1006-1011`

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
8104.1 and 25 at 8107.2
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
`.SetMethod("x", &…` fell from 314 to 34 in 8104.1..8107.2
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
8107.2 and 8109.1
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

### `ui_public_dependencies` → direct deps

Upstream finished removing `browser_public_dependencies` and its `browser/ui`
counterpart, so targets list their direct deps
([1d4b690ab57b](https://chromium.googlesource.com/chromium/src/+/1d4b690ab57b511ef64f9bb681aef7127036573a)).
The rebase routed Brave's UI deps through a new group with the same role
("[cr158] Route the Brave UI public deps through `browser`").

- [ ] `browser/ui/BUILD.gn:39` `ui_public_dependencies` and
      `browser/sources.gni:789` `brave_chrome_browser_public_deps`: move the
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

- [ ] `test/filters/browser_tests.filter:913` and `:1615-1619`
      (`RequestStorageAccessFor*`)
- [ ] `test/filters/browser_tests.filter:2035`
      (`TopLevelStorageExemptionReasonMetricTest`, with its comment)
- [ ] `test/filters/browser_tests.filter:434`
      (`ErrorConsoleBrowserTest.BrowserActionRuntimeError`) and `:1226`
      (`PromoBubbleAccept_OpensDiceSignInTab`)
- [ ] `test/filters/unit_tests.filter:199` (`MaterialTabStripColorMixerTest.*`)
- [ ] Generated filters: rerun
      `tools/chromium_tests_analysis/update-upstream-flake-filters.py`. Entries
      that should drop out: `browser_tests-win.filter:4818`,
      `unit_tests-win-asan.filter:42`, the STTS auto-open suites
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
      the suite became `All/…`)

### Test filters that stopped matching

The Indigo suites became `TEST_P` (`All/…/*`), and their legacy-only cases moved
to new `IndigoLegacyBrowserTest` and `IndigoPageActionControllerLegacyTest`
suites
([8a2bd2103c385](https://chromium.googlesource.com/chromium/src/+/8a2bd2103c385d59c6373503adb3797ddc06dc60)),
so Brave's filters silently re-enable them. Other tests were renamed.

- [ ] `test/filters/browser_tests.filter:794-796`, `:1320-1322`, `:3074-3076`:
      retarget to `All/<Suite>.<name>/*`, and add `IndigoLegacyBrowserTest.*`
- [ ] `test/filters/unit_tests.filter:180`:
      `All/IndigoPageActionControllerTest.*` and
      `IndigoPageActionControllerLegacyTest.*`
- [ ] `test/filters/browser_tests.filter:10`:
      `RecentTabsSubMenuModelWithoutOtherDeviceTabsInAppMenuTest.MaxSessionsAndRecency`
      ([b99eae6c5499f](https://chromium.googlesource.com/chromium/src/+/b99eae6c5499f5bb242e62fa93a6ea9a9e66189e))
- [ ] `test/filters/browser_tests.filter:1395`:
      `SendTabToSelfBubbleControllerBrowserTest.ScrollPositionPropagated_ScrolledPage`
      ([65d99b0dbd18e](https://chromium.googlesource.com/chromium/src/+/65d99b0dbd18ef70fc023a727bae6f7a4afcd4b9))
- [ ] `test/filters/components_unittests.filter:281-283`:
      `GetPreferredAccountForPromo*` → `GetPreferredAccountForSignin*`
      ([69060d1c866bc](https://chromium.googlesource.com/chromium/src/+/69060d1c866bca1bcd1784e0b9c1bf43626f94c1))
- [ ] `test/filters/unit_tests.filter:375`:
      `SendTabToSelfPromoBubbleViewTest.LoadsAccountAwareDesign`
      ([1f3e03da28c3a](https://chromium.googlesource.com/chromium/src/+/1f3e03da28c3a6b766026f70cf55bb3183a77b8e))

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
      `#emailSharedMenu`. Delete it and its import
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

## 5. Optional

### `kUniversalOptOut` is on by default

Upstream enabled `kUniversalOptOut` and set `target_locations` to `us-ca`
([66049b03adbb3](https://chromium.googlesource.com/chromium/src/+/66049b03adbb3)).
`UniversalOptOutServiceFactory` now creates the service in Brave, which records
location history from the variations country and account capabilities. The UI
stays hidden because `kUniversalOptOutSettings` is still off, and Brave has its
own Sec-GPC. Brave only pins `ReadIsSubjectToUniversalOptOutCapability` off
(`base/compile_overridden_features.inc:153`). On iOS,
`IsUniversalOptOutEnabled()` follows `kUniversalOptOut` alone.

- [ ] Pin `universal_optout::features::kUniversalOptOut` disabled with a
      `set_feature_flag_default_state` plaster, and add it to
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

Brave pins `kContextualTasks` and `kContextualTasksCookiePrefetch` disabled, but
two newer flags switch to a rearchitected implementation without checking them.
`IsContextualTasksSidePanelRearchitectureEnabled()` doesn't read
`kContextualTasks`, and `kContextualTasksSidePanelRearchitecture` adds an "AI
Mode" submenu to the tab menu
([87f2181418af6](https://chromium.googlesource.com/chromium/src/+/87f2181418af6)).
`kContextualTasksRearchitecture` makes `TabFeatures::Init` create
`ContextualTasksTabNavigationObserver`. Both are off today.

- [ ] Pin `kContextualTasksSidePanelRearchitecture` and
      `kContextualTasksRearchitecture` disabled

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
Brave appends desktop-only commands there; today only the extension command
conflict check sees them.

- [ ] `chromium_src/chrome/browser/ui/accelerator_table.cc:16-41`: exclude
      `kBraveAcceleratorMap` on Android, or keep only what Android implements

### Android: Ads and Rewards databases trim memory when pressure subsides

Android's `MemoryPressureMonitor` now sends `NONE` when pressure subsides
([f9f29f451c72a](https://chromium.googlesource.com/chromium/src/+/f9f29f451c72a3e505db1553edeacba397ff7658)),
and Brave's listeners ignore the level.

- [ ] `components/brave_ads/core/internal/database/database.cc:461` and
      `components/brave_rewards/core/engine/rewards_database.cc:346`: return
      early on `NONE`, or move to `base::MemoryConsumer`

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
- Android: `kSecondaryGoogleAccountsSigninAllowedOnAndroid` (off), and the
  "Aurora" AI Mode tile in Most Visited
  ([ce7727efc0c6c](https://chromium.googlesource.com/chromium/src/+/ce7727efc0c6c)).
