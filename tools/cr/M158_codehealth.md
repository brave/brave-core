# M158 code health follow-ups

Tasks found by reviewing upstream changes in `157.0.8103.3..158.0.8127.2` (10128
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
- [ ] `browser/brave_tab_helpers.cc:105` `AttachTabHelpers()`: move the helpers
      into `BraveTabFeatures::Init` (`browser/ui/tabs/brave_tab_features.cc`),
      following the per-helper list and idiom in M157's "TabHelpers →
      TabFeatures"; the call from `Init` is only a stopgap. Master has moved the
      Onboarding, SerpMetrics, Ads, CreativeSearchResultAd and WebDiscovery
      helpers so far (`[bedrock]` commits)
- [x] Android: same, in `browser/android/brave_tab_features.cc:35`, including
      Ads and Rewards. "[cr158][Android] Attach Brave tab helpers from
      BraveTabFeatures" calls `brave::AttachTabHelpers()` from the constructor,
      after `CreateSessionServiceTabHelper()`; the same stopgap as desktop
- [ ] `browser/ui/tabs/brave_tab_features.cc:126-127`: the other helpers follow
      discards through `OnWillDiscardContents`, but `rewards_tab_helper_` stays
      bound to the old contents; make it follow discards
      (`ContentsObservingTabFeature`, as `AdsTabHelper` already is)
- [ ] `browser/brave_tab_helpers.cc:183` `AttachPrivacySensitiveTabHelpers()`:
      decide which non-tab WebContents that upstream used to cover still need it
      (prerender, no-state prefetch, extension host, thin webview, overlay
      panel). Only AI Chat calls it now. M157 skipped moving
      `BraveShieldsWebContentsObserver` and `EphemeralStorageTabHelper` for this
      reason; tabs need them too now
- [ ] Add a browser test that a new tab has `BraveShieldsWebContentsObserver`
      and `EphemeralStorageTabHelper`, on desktop and Android
- [ ] Stale references: `browser/PRESUBMIT.py:27,99`,
      `docs/best-practices/architecture.md:1875-1902` (ARCH-074),
      `browser/ui/tabs/brave_tab_features.cc:117-118,124-125` and
      `browser/ui/views/side_panel/ai_chat/ai_chat_side_panel_tab_transfer_bridge.cc:230`

### `settings-toggle-button` became Lit, so Brave's fake-`pref` toggles throw

`settings-toggle-button` and `settings-checkbox` now extend `CrLitElement` on
`PrefKeyObserverMixinLit`
([a57ffb5752066](https://chromium.googlesource.com/chromium/src/+/a57ffb5752066a95f1230ecd751798bccaa053fb)),
after `SettingsBooleanControlMixin` lost its `pref` path
([01b2b4958dae4](https://chromium.googlesource.com/chromium/src/+/01b2b4958dae4416ce63b71b0f5eaa470988194d)).
`willUpdate` asserts `this.pref === this.observedPref_`
(`settings_shared/prefs2/pref_key_observer_mixin_lit.ts:43`), so every Brave
toggle that binds a computed `pref` object throws, as the Tor bridges radios
did. 8117.1 listed this as an upcoming deprecation, after
[9bc6c56d8e5f](https://chromium.googlesource.com/chromium/src/+/9bc6c56d8e5fcbae96ae3c4f9a83192d4fb22b3c)
and
[58d00cfef76d](https://chromium.googlesource.com/chromium/src/+/58d00cfef76d0a9b78ce3e666fc325ad42f4e806)
moved upstream's own fake-`pref` toggles to `pref-key="synthetic.settings.…"`.
Brave's `settings-checkbox` uses already go through `prefKey`.

Move each to a real `pref-key`, to a synthetic pref (a plaster on
`getSyntheticPrefs()` in
`chrome/browser/resources/settings_shared/prefs2/pref_service.ts:57`, which can
share the site-settings plaster; e.g. `secure_dns.ts:289`), or to a plain
`cr-toggle`, as upstream did for the metrics toggle Brave copied
([368fed8692bd](https://chromium.googlesource.com/chromium/src/+/368fed8692bdaca0c7cd873208f61abda378c02d),
M153). All paths are under `browser/resources/settings/`:

- [x] `brave_data_collection_page/brave_data_collection_page.html:4,13,21`; the
      metrics toggle follows upstream's `cr-toggle`
      (`brave_data_collection_page.ts:153-175`)
- [x] `brave_default_extensions_page/brave_default_extensions_page.html:83`
- [x] `brave_default_extensions_page/brave_extensions_manifest_v2_subpage.html:58`
- [x] `brave_leo_assistant_page/brave_leo_assistant_page.html:35`
- [x] `brave_privacy_page/brave_personalization_options.html:122`
- [x] `brave_system_page/brave_vpn_page.html:22`
- [x] `brave_tor_page/brave_tor_subpage.html:48,58,64`
- [x] `brave_wallet_page/brave_wallet_page.html:128`
- [x] `default_brave_shields_page/default_brave_shields_page.html:92,112,138,147,154`
- [x] Add a WebUI test that renders each of these pages without an assertion

Fixed by "[cr158][settings] Stop passing fake pref objects to Lit toggles".

### Contextual Tasks turns on past Brave's `kContextualTasks` pin

[4061337d5d9a3](https://chromium.googlesource.com/chromium/src/+/4061337d5d9a36ee9e09a0985643a47283b3dc9e)
enabled `contextual_tasks::kContextualTasksSidePanel` on desktop and
`lens::features::kLensSidePanelUnification`. `IsContextualTasksUIEnabled()`
(`components/contextual_tasks/public/features.cc:1147`) ORs it with
`kContextualTasks`, so Brave's pin no longer gates the UI. On desktop the
Contextual Tasks and Contextual Tasks UI services are created,
`ContextualTasksNavigationThrottle` is registered
(`chrome_content_browser_client_navigation_throttles.cc:483`), the URL loader
interception and pop-up interception in `chrome_content_browser_client.cc` run,
a pinnable action is added, and every `Tab` creates a `TabUnderlineView`. The
toolbar entry point stays hidden only because `kShowEntryPoint` defaults to
`kNoEntryPoint`.

- [ ] `rewrite/components/contextual_tasks/public/features.cc.yaml`: pin
      `kContextualTasksSidePanel` disabled
- [ ] `app/feature_defaults_unittest.cc:173`: add it
- [ ] Optional: pin `kLensSidePanelUnification`; Brave's `kLensOverlay` and
      `kLensStandalone` pins already gate Lens

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
- [x] The Polymer override forced `#privacy`'s template to always render; the
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

- [x] `chrome/browser/ui/webui/history/browsing_history_handler.cc`: plaster the
      desktop `ShouldShowHistoryPageHistorySyncPromo()` to run the callback with
      `false`, as upstream's Android branch does. Pinning `kUnoPhase2FollowUp`
      off also works, but other sign-in code reads that flag. Fixed by "[cr158]
      Never show the history sync promo on brave://history"
- [x] Add a test that brave://history doesn't render `history-sync-promo` for a
      signed-out profile. `BraveHistorySyncPromoBrowserTest`
- [x] `test/filters/unit_tests.filter:777-778`: the "we don't show the history
      sync promo" comment holds only once the hook lands. Broadened to both
      parameterisations, since the hook also fails the `SignedOut` cases

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

- [x] `rewrite/ui/accessibility/accessibility_features.cc.yaml` (new): pin
      `kReadAnythingOmniboxChip` disabled
- [x] `app/feature_defaults_unittest.cc`: add it

Fixed by "[cr158] Pin `kReadAnythingOmniboxChip` disabled".

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

- [x] Add Brave's categories to `CATEGORY_DEFAULT_SYNTHETIC_PREFS` with a
      plaster on `pref_service.ts`. Pages: `site_settings_autoplay.html`,
      `site_settings_google.html`, `site_settings_ethereum.html`,
      `site_settings_solana.html`, `site_settings_cardano.html` and
      `site_settings_brave_ai.html` in
      `browser/resources/settings/site_settings/`
- [x] Add a WebUI test that each Brave default group shows its value and saves a
      click

Fixed by "[cr158][Settings] Add Brave's categories to the category default
prefs".

### Tor bridges radio buttons throw

`settings-collapse-radio-button` moved to Lit on `PrefKeyObserverMixinLit`
([bff9d99b191c3](https://chromium.googlesource.com/chromium/src/+/bff9d99b191c3945e45272c60e60d4cd22dfc6ee)),
which asserts that `pref` is only ever set through `prefKey`.
`brave_tor_subpage.html:77,90,100` binds a fake `{}` pref on all three bridge
options, a workaround for the removed `PrefControlMixin`, so they throw in
`willUpdate`.

- [x] `browser/resources/settings/brave_tor_page/brave_tor_subpage.html:77,90,100`:
      drop the `pref` bindings. Fixed by "[cr158][Settings] Drop fake pref
      bindings from Tor bridges radio buttons"
- [x] `browser/resources/settings/brave_tor_page/brave_tor_subpage.ts:135-142`:
      drop `useBridgesValuePref_` and its comment. Same commit; the page's
      toggles are in "`settings-toggle-button` became Lit, so Brave's
      fake-`pref` toggles throw"

### On-device speech model toggle doesn't reflect or save its pref

`brave_personalization_options.html:33` binds
`pref="{{prefs.brave.local_ai.on_device_speech_model_enabled}}"`, but the
element has no `prefs` property: "[cr157][settings] Bind privacy toggles by
pref-key" moved it off `prefs`, and the binding came in later from master's
3a7aaa6f005 ("[On-Device AI] [WebSpeech] Require user consent for downloading
speech model"). The toggle shows off and doesn't save. It's the last `{{prefs.`
binding in Brave, and the range removed the last `<settings-prefs>`
([ff55acf07c8a](https://chromium.googlesource.com/chromium/src/+/ff55acf07c8abb6e502e34fdefb75fd5486f3bed)).

- [x] `browser/resources/settings/brave_privacy_page/brave_personalization_options.html:33`:
      `pref-key="brave.local_ai.on_device_speech_model_enabled"` (already
      allowlisted at
      `browser/extensions/api/settings_private/brave_prefs_util.cc:341`)
- [x] Add a WebUI test that the toggle reflects and saves the pref

Fixed by "[cr158][settings] Stop passing fake pref objects to Lit toggles",
which reads and writes the local-state pref through `chrome.settingsPrivate` (it
can't be bound by `pref-key`).

### iOS: upstream's app-specific-pages rule list applies again

Upstream renamed `CreateLocalBlockingJsonRuleList()` to
`CreateAppSpecificURLsJsonRuleList()`, dropped `file://` from it and added the
`document` resource type
([e7777f06181e1](https://chromium.googlesource.com/chromium/src/+/e7777f06181e1e1a023dc309eee11da3bd9ec362)).
`chromium_src/ios/web/web_state/ui/wk_content_rule_list_util.mm` still defines
the old name to return an empty list, so it matches nothing and WebUIs are
blocked from loading external resources and documents again. The same commit
makes `favicon_util.mm` reject http(s) favicons on app-specific pages.

- [x] Retarget the `#define` to `CreateAppSpecificURLsJsonRuleList`. Fixed by
      "[cr158][iOS] Restrict external favicons and subresources on app-specific
      pages"
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
- [x] `chromium_src/ios/chrome/browser/signin/model/about_signin_internals_factory.cc:15-31`:
      upstream converted `AboutSigninInternalsFactory` too
      ([61af71ca018a5](https://chromium.googlesource.com/chromium/src/+/61af71ca018a5150f0ea1f042637131d493656bf)),
      and Brave's full-file replacement still defines the default constructor,
      destructor, `GetForProfile()` and `GetInstance()`. Keep only the `PassKey`
      constructor, the null `BuildServiceInstanceFor()` and
      `RegisterProfilePrefs()`, as for `GoogleGroupsManagerFactory`. No other
      `chromium_src/ios` factory replaces the whole file. Fixed by "[cr158][ios]
      Adapt `AboutSigninInternalsFactory` override to typed factory"

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

- [x] `ios/browser/api/history/brave_history_api.mm:216-217,225-226`
- [x] `ios/browser/api/history/importer/brave_history_importer.mm:39-40`
- [x] `ios/browser/safari_data_import/safari_data_importer_coordinator.mm:54-55`
- [x] `ios/browser/ui/webui/ai_chat/ai_chat_ui.mm:186-187`
- [x] `chromium_src/ios/chrome/browser/sync/model/sync_service_factory.mm:16-17`:
      upstream now calls `GetForProfile(profile)` there too
- All five fixed by "[cr158][ios] Drop `ServiceAccessType` from
  `HistoryServiceFactory` calls"
- [ ] The sync factory's call can't return null now; check that
      `BraveSyncServiceImplDelegate` doesn't rely on null

### iOS: Brave's 15 service factories include a header that moved to `internal/`

[cd096f96fa7e](https://chromium.googlesource.com/chromium/src/+/cd096f96fa7ef101cee8adea5571ca810aac2dda)
moved `profile_keyed_service_factory_ios.h`, the refcounted factory,
`profile_dependency_manager_ios.h` and `profile_keyed_service_utils.h` to
`ios/chrome/browser/shared/model/profile/internal/`, whose target is visible
only to `:profile_keyed_service_factory`, `profile/test` and its tests.
`:profile_keyed_service_factory` now holds only the typed headers. Every Brave
iOS factory includes the old path, so the iOS build breaks, and moving to
`TypedProfileKeyedServiceFactoryIOS` is now required. Upstream finished its own
conversion in this milestone (3cc80ea5bcdc3, Bug 570433691; 3 classic and 239
typed at 8117.1), and
[f274634e4a1c](https://chromium.googlesource.com/chromium/src/+/f274634e4a1c0229f06413ba8731f0768a80f6c2)
marked typed factories `final`. The idiom is `explicit Factory(PassKey key)`
calling `TypedProfileKeyedServiceFactoryIOS(std::move(key), "Name")`, with
generated `GetForProfile()` and `GetInstance()`. Re-check with
`git grep -n 'profile/profile_keyed_service_factory_ios.h' -- ios chromium_src/ios`.
All paths are under `ios/browser/`:

- [x] `ai_chat/ai_chat_service_factory.h:13`,
      `ai_chat/model_service_factory.h:12`,
      `ai_chat/tab_tracker_service_factory.h:12`
- [x] `api/webcompat_reporter/webcompat_reporter_service_factory.h:12`
- [x] `brave_account/brave_account_service_factory_ios.h:12`
- [x] `brave_ads/ads_service_factory_ios.h:11`; `brave_ads/ads_tab_helper.mm:19`
      includes the old header without using it
- [x] `brave_origin/brave_origin_service_factory.h:12`
- [x] `brave_shields/brave_shields_settings_service_factory.h:13`
- [x] `brave_wallet/brave_wallet_service_factory.h:12`
- [x] `debounce/debounce_service_factory+private.h:12`
- [x] `favicon/brave_ios_favicon_loader_factory.h:12`; also name its testing
      factory (`.h:26`, `.mm:49-51`) as the typed factory's `TestingFactory` or
      `KeyedServiceTestingFactoryIOS`
      ([992f5e4dc610](https://chromium.googlesource.com/chromium/src/+/992f5e4dc6105b0f41c4db84775ea4c5169b86f8),
      [c89e7d662d1eb](https://chromium.googlesource.com/chromium/src/+/c89e7d662d1eb0d80fb16e4835b2371171ed3fa5))
- [x] `misc_metrics/profile_misc_metrics_service_factory.h:12`
- [x] `serp_metrics/serp_metrics_service_factory_ios.h:11`
- [x] `skus/skus_service_factory.h:12`
- [x] `url_sanitizer/url_sanitizer_service_factory+private.h:12`
- All fixed by "[cr158][ios] Convert factories to
  `TypedProfileKeyedServiceFactoryIOS`"; the re-check grep finds nothing

### iOS: `CSPDirectiveName::TreatAsPublicAddress` removed

[62b4de517e442](https://chromium.googlesource.com/chromium/src/+/62b4de517e442f164d27559461fcfd1fa9f3616b)
removed the directive from the mojom enum.

- [x] `ios/web/webui/brave_url_data_source_ios.mm:109`: drop the `case`. Fixed
      by "[cr158][ios] Remove `TreatAsPublicAddress` CSP directive case"

### iOS: `CSVFieldParser` lost `HasMoreFields()` and `NextField(std::string_view*)`

[5098255c30e76](https://chromium.googlesource.com/chromium/src/+/5098255c30e76e5c7c3789a7ed2474807b032d64)
replaced them with
`std::optional<base::expected<std::string_view, SyntaxError>> NextField()`.
Brave's Safari CSV fork is only built from
`ios/browser/api/password/importer/BUILD.gn:17-18`, so only the iOS build
breaks. Upstream's `csv_password.cc` shows the new loop.

- [ ] `components/password_manager/core/browser/import/csv_safari_password.cc:73-77`
      and `csv_safari_password_sequence.cc:108-112`:
      `while (auto field = parser.NextField())`, with `!field->has_value()` as
      the syntax error

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

- [x] `patches/third_party-blink-renderer-core-html-html_capability_element_base.cc.patch`:
      check `descriptor->detail->which() == kUserMedia` before `Bind()`, using
      the existing `descriptor`
- [x] Drop the comment edit

Already fixed on the branch: the patch is a single hunk after `descriptor` is
computed and leaves upstream's comment alone.

### Component updater crashes on Brave's ECDSA CUP key

> mherrmann's "fixup! [cr158][WIP] ECDSA CUP verification removed" switches
> `request_sender.cc` to a Brave ML-DSA-44 key and drops the `CHECK` patch and
> the updater's CUP-signing override. Don't reintroduce the ECDSA key.

Upstream removed ECDSA CUP verification and `kPqcCupSigning`; `Cup::Cup()` now
does `CHECK(public_key->IsMldsa44())` and key IDs are `ML-DSA-44-<n>`
([b5589ebe1d1bd](https://chromium.googlesource.com/chromium/src/+/b5589ebe1d1bda6ca77af7ec34706e1317b6d7bd)).
Brave's plaster makes `BuildSigner()` return a `Cup` built from
`kBravePublicKey`, a P-256 SPKI. `RequestSender`'s constructor always builds the
signer, so the first update check hits the `CHECK`. Component updates disable
CUP signing, but the Omaha 4 macOS updater uses this key.

Until the new key lands, "[cr158][WIP] ECDSA CUP verification removed" keeps
things working: `patches/components-client_update_protocol-cup.cc.patch`
comments out the `IsMldsa44()` `CHECK`, and
`rewrite/chrome/updater/configurator.cc.yaml` disables CUP signing for the
updater, since signed responses can't validate.

- [x] mherrmann's change: switch
      `chromium_src/components/update_client/request_sender.cc:14-26` to the
      Brave ML-DSA-44 key, re-enable updater CUP signing, and drop the `CHECK`
      patch
- [ ] `rewrite/components/update_client/request_sender.cc.yaml`: update the
      description, which still describes upstream's ECDSA/ML-DSA flag
- [x] `chromium_src/components/update_client/request_sender_unittest.cc`: the
      `cup2key=1:` expectations and `BraveKeyIgnoresPqcCupSigningFlag` refer to
      the old format and the deleted flag
- [ ] `test/filters/components_unittests.filter:265-269`: re-evaluate
      `-All/RequestSenderTest.CupKeySelection/*` and its comment once the new
      key lands

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

- [x] Confirm in a build that `'LanguageDetector' in self` and
      `'Translator' in self` are true
- [x] `renderer/brave_content_renderer_client.cc:149`: disable them with
      `EnableFeatureFromString`, as for `AIDecisionModelAPI`, or set the json5
      `status` instead of `base_feature_status`
- [x] `app/feature_defaults_unittest.cc:367`: also check the runtime feature,
      not only the base feature

Already fixed by "[cr157] Disable the Translator and LanguageDetector web APIs"
(`46a3fdc7e99`); `TranslationWebAPIsBrowserTest.NotExposedToPages` covers it in
place of the unittest change.

### "Relaunch to update" skips Brave's update dialog

Upstream removed `kFewerUpdateConfirmations`, and `MaybeUpdateChrome()` now
always calls `chrome::AttemptRelaunch()` on Windows, Mac and Linux
([206e587624a3d](https://chromium.googlesource.com/chromium/src/+/206e587624a3d40a381bf5250cb5f134c8aadae1)).
Brave had pinned the flag off because relaunching straight from the menu command
left the updated browser not starting or crashing (master 91f761e44c5); "[cr158]
Drop the kFewerUpdateConfirmations pin" dropped the pin without replacing it.

- [x] Test "Relaunch to update" on Mac (Sparkle) and Windows. If the old failure
      is back, hook `chrome::MaybeUpdateChrome` to show the dialog
- [x] `browser/ui/views/frame/brave_browser_view.cc:652`
      `ShowUpdateChromeDialog()`: unreachable on desktop; remove it with
      `browser/ui/views/update_recommended_message_box_mac.{h,mm}` and
      `patches/chrome-browser-ui-views-update_recommended_message_box.h.patch`,
      or re-wire it

No intervention needed: `chrome::AttemptRelaunch()` is `AttemptRestart()`, which
Brave already routes through Sparkle on macOS (c53f5adb90c, after the 2024 pin).
`RelaunchToUpdate*` browser tests cover it; the dead dialog override was dropped
in "[cr158] Drop the kFewerUpdateConfirmations pin".

- [ ] Build and run `SparkleGlueTest.*` (`brave_unit_tests`) and
      `RelaunchToUpdateSparkleBrowserTest.*` (`brave_browser_tests`) on a
      non-component macOS build; they have never been compiled

### Hovered inactive tabs lose Brave's tab text colours

`TabStyle::GetTabForegroundColor()` now reads
`kColorTabForegroundInactiveHoverFrame{Active,Inactive}` for hovered inactive
tabs, which `AddTabStripColorMixer` defaults to the non-hover IDs
([33a6011aae094](https://chromium.googlesource.com/chromium/src/+/33a6011aae094df378af3489de54e8109207c8c0)).
References resolve through `GetLastNonPostprocessingMixer()`, and Brave sets the
non-hover IDs in a postprocessing mixer (70% opacity, and the darker theme), so
hovered tabs render without them.

- [x] `browser/ui/tabs/brave_tab_color_mixer.cc:220-253`: set the two hover IDs
      in the postprocessing mixer, as for the non-hover ones
- [x] `browser/ui/views/tabs/tab_style_views_unittest.cc`: add a hovered
      inactive tab case

Fixed by "[cr158][Tabs] Keep Brave's tab text colour on hovered inactive tabs";
the tests went into `browser/ui/color/brave_color_mixers_unittest.cc`, which
runs Brave's mixers, for the default and darker themes.

### Polymer settings selects lose Brave's `md-select` style

Polymer `cr_elements/md_select.css` used to be generated from
`md_select_lit.css`, so Brave's override of the latter
(`chromium_src/ui/webui/resources/cr_elements/md_select_lit.css`) styled both.
Upstream deleted it and added a standalone copy at
`chrome/browser/resources/settings_shared/md_select.css`
([cda52a812cc74](https://chromium.googlesource.com/chromium/src/+/cda52a812cc74f94d39f1ca347560b7895f45c03)),
which Brave doesn't override. "[cr158] Follow WebUI shared Polymer files move in
settings" only fixed the imports, so Brave's Polymer pages that
`include="md-select"` (e.g. `brave_content_page/content.html`,
`brave_leo_assistant_page/*.html`, `brave_new_tab_page.html`,
`social_blocking_page.html`) render upstream's select.

- [x] Add `chromium_src/chrome/browser/resources/settings_shared/md_select.css`
      that includes upstream's and applies Brave's rules from
      `md_select_lit.css`
- [x] Delete `chromium_src/ui/webui/resources/cr_elements/md_select.css`, whose
      target is gone
- [x] Drop the
      `<link rel="import" href="chrome://resources/cr_elements/md_select.css.html">`
      lines in `brave_new_tab_page.html:1`,
      `brave_personalization_options.html:1` and `social_blocking_page.html:1`

Already fixed by "[cr158] Move `md_select.css` override to `settings_shared`".
`BraveMdSelectStyle` (in `settings_toggle_button_test.ts`) checks that a Brave
Polymer page's select gets Brave's rules on top of upstream's.

### Rounded WebUI icons override Brave's edit and folder icons

[c11ad0021df3e](https://chromium.googlesource.com/chromium/src/+/c11ad0021df3ea0608b099ad8b52ad1c0665d47e)
enabled `kWebUIRoundedIcons`, which sets `[webui-rounded-icons]` on every WebUI.
That turns on the `:host-context([webui-rounded-icons])` block in
`cr_icons_lit.css`, whose `.icon-edit` and `.icon-folder-open` rules are more
specific than Brave's plain ones in
`chromium_src/ui/webui/resources/cr_elements/cr_icons_lit.css:13-19`, so
upstream's icons win (bookmarks folders, search engine and downloads entries).
`cr_icon.ts:133` already handles the rounded icon names.

- [ ] `chromium_src/ui/webui/resources/cr_elements/cr_icons_lit.css`: also apply
      both rules under `:host-context([webui-rounded-icons])`

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

### Android: page info moves into the app menu

[a034bd17f266](https://chromium.googlesource.com/chromium/src/+/a034bd17f2667dd904986037f42f84ce937e655c)
enabled `kAndroidPageInfoAsAppMenuItem` and `IPH_SiteControls`, and the Java
`newCachedFlag(ANDROID_PAGE_INFO_AS_APP_MENU_ITEM, true)` default with them.
Brave never pinned the flag: master 75f3596e528 only asserted upstream's default
in `app/feature_defaults_unittest.cc:163`, because the page info icon "gets
lost" once it leaves the toolbar. The icon now leaves the address bar for a
"Site controls" menu item, the IPH highlights it, and that Android test fails.
`base/compile_overridden_features.inc:16` lists the feature, but nothing
overrides it.

- [x] Pin it or adopt it: adopted by "[cr158][Android]
      kAndroidPageInfoAsAppMenuItem enabled by default" (Site controls stays out
      of the Customize menu, and its icon stays in the icon row) and
      "[cr158][Android] Use the Nala tune-small icon for Site controls". That
      commit dropped the test entry rather than moving it to the enabled list
- [x] `base/compile_overridden_features.inc:16`: drop
      `AndroidPageInfoAsAppMenuItem`, which nothing overrides
- [x] Decide on `kIPHSiteControlsFeature`
      (`components/feature_engagement/public/feature_constants.cc:623`) It is
      shown, but has no any Chromium-branded items.
- [x] `android/junit/src/org/chromium/chrome/browser/tabbed_mode/BraveTabbedAppMenuPropertiesDelegateUnitTest.java:473`:
      the "disabled for the whole class" comment is stale now that the flag
      stays on

### Android: Brave's NTP toolbar side padding no longer applies

Upstream removed `OmniboxResourceProvider.getToolbarSidePaddingForNtp()`, and
`ToolbarPhone` now reads `R.dimen.toolbar_edge_padding_ntp` (16dp) directly
([78fd9a6f89e22](https://chromium.googlesource.com/chromium/src/+/78fd9a6f89e22cec8da707c8c536ed62d4158c46)).
Brave's `changeMethodOwner` redirect to `BraveOmniboxResourceProvider` (8dp,
`brave_toolbar_edge_padding_ntp`) matches nothing, and the `BytecodeTest`
`methodExists` check fails. This is what M158's "`OmniboxResourceProvider`
static methods" deprecation item expected.

- [x] Restore the 8dp padding: plaster `mToolbarSidePaddingForNtp` in
      `chrome/browser/ui/android/toolbar/java/src/org/chromium/chrome/browser/toolbar/top/ToolbarPhone.java:360`
      to use `R.dimen.brave_toolbar_edge_padding_ntp`
      (`android/java/res/values/brave_dimens.xml:52`)
- [x] Delete
      `build/android/bytecode/java/org/brave/bytecode/BraveOmniboxResourceProviderClassAdapter.java`,
      its entries in `build/android/bytecode/BUILD.gn:109` and
      `BraveClassAdapter.java:109`,
      `android/java/org/chromium/chrome/browser/omnibox/styles/BraveOmniboxResourceProvider.java`
      and `android/brave_java_sources.gni:297`
- [x] `android/javatests/org/chromium/chrome/browser/BytecodeTest.java:1000-1006`:
      drop the assertion
- [x] `android/java/apk_for_test.flags:974-976`: drop the keep rule

### Android: the Homepage row moved to Appearance

With `kAndroidBottomBar` on (its `move_toolbar_settings_to_appearance` param
defaults to true), the `homepage` row is inflated by
`appearance_preferences.xml` and `MainSettings` drops its own
([0a3ed81a508b3](https://chromium.googlesource.com/chromium/src/+/0a3ed81a508b3)).
Brave's homepage handling is keyed on the `MainSettings` row, so the Appearance
row opens upstream's `HomepageSettings`. With the flag off, Brave's copied
search index provider lacks upstream's new `removeEntryForKey(PREF_HOMEPAGE)`,
so settings search lists a Homepage row under Appearance that isn't there.

- [x] `android/java/org/chromium/chrome/browser/settings/AppearancePreferences.java:84`:
      point the inflated row at `BraveHomepageSettings` and apply Brave's hide
      rule, as `BraveMainPreferencesBase.java:365,439,534` does for Main. Fixed
      by "[cr158][Android] Adapt the rows kAndroidBottomBar moves into
      Appearance"
- [x] `AppearancePreferences.java:482-560` `SEARCH_INDEX_DATA_PROVIDER`: the
      `PREF_HOMEPAGE` removal landed in "[cr158][Android] Fix duplicate
      Appearance entries in the settings search index" (unconditionally, where
      upstream removes it only with the row hidden). The `PREF_TAB_POSITION` and
      bookmark bar removals and the "Address bar shortcut" retitling landed in
      "[cr158][Android] Sync the Appearance search index with the shown rows".
      The bookmark bar half predates the branch and is on master too, where the
      two rows leave a search entry that goes nowhere:
      [#60312](https://github.com/brave/brave-browser/issues/60312), fixed by
      [#41090](https://github.com/brave/brave-core/pull/41090)
- [x] `AppearancePreferences.java:420` `applyOrdering()` has no entry for
      `tab_position` or `bookmark_bar_switch`, so both keep their upstream XML
      orders (6 and 4) and tie with `homepage` and `toolbar_shortcut`. On a tie
      `Preference.compareTo` sorts by translated title, and order 4 also lands
      the bookmark bar row above the General header. Not cr158 work: both rows
      are already in Appearance on Nightly, iconless and unfinished upstream, so
      there is nothing shippable to order correctly yet. Revisit when upstream
      finishes them
- [x] `AppearancePreferences.java:542`: upstream also puts the bookmark bar
      visibility state in the search entry's summary, via the package-private
      `AppearanceSettingsFragment.getBookmarkBarVisibilityStateSummaryRes()`.
      Mirroring it needs a visibility plaster for a cosmetic, flag-gated,
      tablet-only string. Pre-existing on master, so fixed there rather than on
      the branch: [#60313](https://github.com/brave/brave-browser/issues/60313),
      fixed by [#41091](https://github.com/brave/brave-core/pull/41091)

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
- [x] `android/java/proguard.flags:70-72`: the `showUndoBar(...)` keep rule only
      served the deleted reflection call; delete it

### Android: `OriginalProfileSupplier` removed

Upstream deleted the class rather than migrate it, since it assumes one regular
profile per process
([a1ca906126779](https://chromium.googlesource.com/chromium/src/+/a1ca906126779f4b0e37167dca13c48fb39deec1)).
Brave uses it to apply its default search engine once the regular profile
exists, which breaks the Android build.

- [x] `android/java/org/chromium/chrome/browser/BraveApplicationImplBase.java:19,71`
- [x] `android/java/org/chromium/chrome/browser/referrer/BraveReferrer.java:29,223-232`
- Both fixed by "[cr158] Replace removed OriginalProfileSupplier with
  BraveProfileUtils" (`BraveProfileUtils.onRegularProfileAvailable`)

### Android: `GlicButtonDelegate` moved to `glic.toolbar_button`

[1b4c81d02f73b](https://chromium.googlesource.com/chromium/src/+/1b4c81d02f73b96529bac87a6ffe825b1a900c3a)
split `//chrome/browser/glic/android` into subpackages. `GlicEnabling` stayed,
and no Brave XML names a moved class.

- [x] Update the import in
      `android/java/org/chromium/chrome/browser/tabbed_mode/BraveTabbedAdaptiveToolbarBehavior.java:20`,
      `android/java/org/chromium/chrome/browser/toolbar/BraveToolbarManager.java:53`,
      `android/javatests/org/chromium/chrome/browser/BytecodeTest.java:86` and
      `android/junit/src/org/chromium/chrome/browser/tabbed_mode/BraveTabbedAdaptiveToolbarBehaviorTest.java:31`.
      Fixed by "[cr158][Android] GlicButtonDelegate moved"

### Android: `product_config_java_packages` no longer exists

[fde64a8deb78f](https://chromium.googlesource.com/chromium/src/+/fde64a8deb78fdf0efdff5127444225cba6527a6)
folded `ProductConfig` into `BuildConfig`, and `ResourceBundle` reads
`BuildConfig.LOCALES` itself. GN fails on the unused assignment.

- [x] `test/BUILD.gn:1661-1666`: drop the assignment and its comment. Fixed by
      "[cr158][Android] Drop product_config_java_packages from
      brave_browser_tests"

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

### Android: `LocationBarCoordinator` takes an `OmniboxResourceProvider`

The `Function<Tab, Bitmap> tabFaviconFunction` parameter became
`OmniboxResourceProvider resourceProvider`, which `ToolbarManager` and
`SearchUiCoordinator` now own
([c66ae8711c3c](https://chromium.googlesource.com/chromium/src/+/c66ae8711c3cd1264fd0da880a2b7a88d3aeea06)).
Brave's `redirectConstructor` target still takes the `Function`.

- [x] `android/java/org/chromium/chrome/browser/omnibox/BraveLocationBarCoordinator.java:93,136`:
      take and pass the provider; drop the `Bitmap` (`:10`) and `Function`
      (`:44`) imports
- [x] `android/javatests/org/chromium/chrome/browser/BytecodeTest.java:1915`:
      `Function.class` → `OmniboxResourceProvider.class` in `constructorsMatch`

### Android: `SEND_TAB_TO_SELF_EXTRA_ENTRY_POINTS` removed

[28139893f60a](https://chromium.googlesource.com/chromium/src/+/28139893f60ac3376d29a00c244941a55ffa6861)
removed the flag, which was on, and its `ChromeFeatureList` constant. Brave's
behaviour doesn't change, but two junit tests disable the constant.

- [x] `android/junit/src/org/chromium/chrome/browser/tasks/tab_management/BraveTabGridContextMenuCoordinatorTest.java:66`:
      drop the `@DisableFeatures`
- [x] `android/junit/src/org/chromium/chrome/browser/toolbar/BraveToolbarLongPressMenuHandlerUnitTest.java:51`:
      drop it; the handler is built with `url -> true` (`:94`), so
      `testbuildMenuItemsWithUrl` (`:126`) now gets a fourth, "Send to devices"
      item

### Android: `kPopularSitesJsonPref` moved to a private header

The popular sites prefs moved to
`components/ntp_tiles/popular_sites_pref_names.h`, whose target is visible only
inside `//components/ntp_tiles`
([6c240d5b67c85](https://chromium.googlesource.com/chromium/src/+/6c240d5b67c857fdd2239333b74a2701fdbcf461)).
Brave's override is likely redundant:
`chromium_src/components/ntp_tiles/most_visited_sites.cc` already forces
`popular_sites_` to null.

- [ ] `browser/brave_profile_prefs.cc:73,191-192`: drop the
      `kPopularSitesJsonPref` default and the include. "[Android][cr158] Ntp
      tiles pref names moved and restricted visibility" kept it by widening the
      target's visibility with a `components/ntp_tiles/BUILD.gn` plaster; drop
      that plaster and the dep (`browser/BUILD.gn:94`) too

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

- [x] `chromium_src/components/omnibox/browser/suggestion_group_util.cc:11-29`:
      wrap `BuildDefaultGroupsForInput` instead, or plaster the group into
      upstream's `BuildDefaultGroups()`; drop the dead wrapper
- [x] `components/omnibox/browser/commander_provider_unittest.cc:366`: also
      check that `GROUP_OTHER_NAVS` has a non-empty header

Fixed by "[Omnibox][cr158] Show the quick commands header again", which replaces
the `#define` with a plaster on upstream's `GROUP_OTHER_NAVS` entry.
`CommanderProviderTest.AmbientModeGroupHasQuickCommandsHeader` covers it.

### Pre-existing: ambient commander matches wait for another provider's update

`CommanderProvider::OnCommanderUpdated()` only calls `NotifyListeners()` when
`!done()`, but `Start()` never clears `done_`, which defaults to `true`. So the
commands that arrive after a typed query (without the `:>` prefix) only reach
the omnibox when another provider updates the result. In a browser test, typing
"New tab" with commander suggestions on produced five commands in
`CommanderService` and none in the omnibox. The `if (!done())` guard came with
the Chromium 122 upgrade (a923a2e3082).

- [ ] `components/omnibox/browser/commander_provider.cc`: set `done_ = false` in
      `Start()` while waiting for the delegate, and back to `true` when the
      matches arrive
- [ ] Add a browser test that typed text shows commands in the omnibox result,
      under the quick commands header

## 2. Upcoming deprecations

### `TOP_LEVEL_STORAGE_ACCESS` content setting

Upstream removed `document.requestStorageAccessFor()` and says the rest of the
`top_level_storage_access` code follows
([064bca2b1e8bc](https://chromium.googlesource.com/chromium/src/+/064bca2b1e8bc9dfdd45d6f6175b67df938e52b7)).
Removing the enum value will break the build. At 8117.1 the value is still
there, after
[75b7b36f86d9](https://chromium.googlesource.com/chromium/src/+/75b7b36f86d948a0210e2f75ed6d5dd968508ec0)
removed its network plumbing and
[845d377ebe5d](https://chromium.googlesource.com/chromium/src/+/845d377ebe5dc8e48537ff830fcc60e85585b072)
the Related Website Sets internals, which says the remaining stubs follow.

- [ ] `chromium_src/components/content_settings/core/browser/content_settings_registry.cc:334-357`:
      drop Brave's re-registration of
      `ContentSettingsType::TOP_LEVEL_STORAGE_ACCESS` when the type goes
- [ ] `components/privacy_sandbox/brave_privacy_sandbox_settings.h:42` and
      `.cc:21-50`: drop the `AreRelatedWebsiteSetsEnabled()` override and the
      `kPrivacySandboxRelatedWebsiteSetsEnabled` write when upstream removes
      them

### `View::DeprecatedLayoutImmediately()` is banned by presubmit

[b5cc6f05dfcf9](https://chromium.googlesource.com/chromium/src/+/b5cc6f05dfcf91dcbfd47c38b4e5067ee458fab7)
added a `_BANNED_CPP_FUNCTIONS` rule with `treat_as_error=True`, and a
`DeprecatedLayoutImmediatelyPassKey` overload open only to four upstream
classes; the no-argument overload will be removed. Brave keeps
`CheckNoBannedFunctions` on, so any Brave change touching these lines fails
presubmit. Upstream's count is still high: 280 to 272
(`git grep -E 'DeprecatedLayoutImmediately\(\)' <tag> -- '*.cc' '*.mm' '*.h' | wc -l`).
Use `InvalidateLayout()`; tests add `views::test::RunScheduledLayout()`. All
paths are under `browser/ui/views/`:

- [ ] `brave_actions/brave_actions_container.cc:142`
- [ ] `frame/brave_browser_view.cc:470`
- [ ] `frame/vertical_tabs/vertical_tab_strip_region_view.cc:779`
- [ ] `location_bar/brave_location_bar_view.cc:318`
- [ ] `sidebar/sidebar_items_scroll_view.cc:228` (on `parent()`)
- [ ] `tabs/brave_tab_strip.cc:364`
- [ ] `toolbar/brave_toolbar_view.cc:494`
- [ ] 13 test sites:
      `git grep -n 'DeprecatedLayoutImmediately()' -- '*test*.cc'`

Deferred until upstream's migration settles; revisit at the next bump. As of
8127.2 upstream has only migrated tests (`views::test::RunScheduledLayout()`)
and calls that nothing reads layout after (`InvalidateLayout()`); the classes
that need synchronous layout got `DeprecatedLayoutImmediatelyPassKey`, which is
upstream-only. There is no support yet for running code after a deferred layout.
For Brave:

- Five of the seven product sites can take `InvalidateLayout()`: nothing after
  them reads bounds (`brave_location_bar_view.cc` and `brave_toolbar_view.cc`
  only `SchedulePaint()` after)
- `sidebar_items_scroll_view.cc:228` needs the layout to have run:
  `OnItemAdded()` then calls `NeedScrollForItemAt()` to choose between scrolling
  and the feedback bubble (`SidebarBrowserTest.ItemAddedScrollTest`,
  `ItemAddedBubbleAnchorViewTest`)
- `brave_tab_strip.cc:364` is for the initial tabs layout; check with the
  vertical tab tests

### Static key pin checks move into cert verification

[5691202e21434](https://chromium.googlesource.com/chromium/src/+/5691202e214341a5d96a3296c1b9811cce19572b)
split key pinning out of `TransportSecurityState` into `KeyPinningStateChrome`,
and says follow-up CLs move enforcement into cert verification. Brave iOS calls
`TransportSecurityState::CheckPublicKeyPins()` directly and relies on
`chrome_key_pinning_supported` (`build/args/ios_defaults.gni:22`).

- [ ] `ios/browser/api/net/certificate_utility.mm:195`: follow
      `CheckPublicKeyPins` when enforcement moves

### Password Manager WebUI is moving to `PrefService`, then Lit

[a876bfec3efdc](https://chromium.googlesource.com/chromium/src/+/a876bfec3efdc)
calls itself "a preparatory step in migrating to PrefService and ultimately
migrating to Lit". Brave's Polymer overrides of `password-manager-side-bar` and
`site-favicon` will silently stop applying when those elements go Lit.

- [ ] `chromium_src/chrome/browser/resources/password_manager/side_bar.ts` and
      `site_favicon.ts`: port to lit_manglers when the elements become
      `CrLitElement`

### WebUI lint against re-exports will widen

The `noReExport` ESLint rule (Bug 576312225,
[c3747630714f4](https://chromium.googlesource.com/chromium/src/+/c3747630714f4),
[245999ae218db](https://chromium.googlesource.com/chromium/src/+/245999ae218db))
checks only named re-exports in files that define a `CrLitElement`, so Brave's
`export * from './X-chromium.js'` override idiom still passes.

- [ ] `chromium_src/chrome/browser/resources/tab_group_shared/search.ts:9`: its
      `export type {…} from './search-chromium.js'` fails once the rule covers
      all non-barrel files; also watch whether `export *` gets flagged (about 30
      `chromium_src` WebUI overrides)

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
8104.1, and 25 at 8107.2 through 8117.1; 26 at 8127.2, so the campaign has
stalled
(`git grep -E 'Writer::Write\(cbor::Value\(' <tag> -- '*.cc' '*.mm' | wc -l`).

- [x] `components/brave_wallet/browser/cardano/cardano_cip30_serializer.cc:35,56,83,105`
- [x] `components/brave_wallet/browser/fil_transaction.cc:271`

Done in "[cr158][Wallet] Write CBOR maps and arrays without a `cbor::Value`";
Brave has no `Writer::Write(cbor::Value(` left.

### gin `SetMethod<&T::M>()` compile-time form

gin's `ObjectTemplateBuilder` now prefers `SetMethod<&T::M>("m")`,
`SetProperty<&T::Get>("p")` and `SetLazyDataProperty<&T::Get>()` for anything
that isn't a bound callback; they skip the per-call callback holder
([76772fc798ced](https://chromium.googlesource.com/chromium/src/+/76772fc798cedbbf37df6c25dc87ff4c63c31cab),
e.g.
[50940ba33265f](https://chromium.googlesource.com/chromium/src/+/50940ba33265fcd8808b860d6f3869b60a052ce2)).
`.SetMethod("x", &…` fell from 314 to 34 in 8104.1..8107.2, and to 31 at 8111.0
through 8127.2
([0ade64b5d6157](https://chromium.googlesource.com/chromium/src/+/0ade64b5d6157e247ff7978a0e570746a88a7aae))
(`git grep -E '\.SetMethod\("[^"]+", *&' <tag> -- '*.cc' | wc -l`).

- [x] `components/brave_wallet/renderer/js_cardano_provider.cc:76-77`
- [x] `components/brave_wallet/renderer/js_cardano_wallet_api.cc:76-87`
- [x] `components/brave_wallet/renderer/js_ethereum_provider.cc:305`, `:403-413`
- [x] `components/brave_wallet/renderer/js_polkadot_provider.cc:61`: a
      `.SetProperty("version", &…)`, which the count misses; use
      `SetProperty<&…>`
- [x] `components/brave_wallet/renderer/js_solana_provider.cc:185-197`
- [x] `components/skus/renderer/skus_js_handler.cc:315-320`
- [x] `components/speedreader/renderer/speedreader_js_handler.cc:76-77`

Done in "[cr158] Bind gin methods and properties at compile time", which also
converts the calls the count misses (multi-line ones and those keyed by
constants, e.g. `kIsBraveWallet`); Brave has no runtime-form `SetMethod` or
`SetProperty` left.

### gtest `InvokeWithoutArgs(f)` → `f`

The single-argument `InvokeWithoutArgs` is deprecated in upstream googletest,
and nine scripted commits under Bug 575681193 pass the callable directly (e.g.
[1de614befad5](https://chromium.googlesource.com/chromium/src/+/1de614befad52e0cd014a65377e444523bdb7761):
`.WillOnce(InvokeWithoutArgs([&] {…}))` → `.WillOnce([&] {…})`). The
two-argument `(obj, &Method)` form stays. Single-argument uses fell from 460 to
260 in 8115.1..8117.1, and to 26 at 8127.2 (a balanced-paren scan of
`*.cc *.mm *.h`).

- [x] `components/ai_chat/core/browser/conversation_handler_unittest.cc:3663,3971,3996,4416,4430,5292,7367,7831,7963`
- [x] `components/brave_vpn/browser/brave_vpn_service_impl_unittest.cc:1186`

Done in "[cr158] Pass gMock actions without `InvokeWithoutArgs`"; Brave has no
single-argument uses left. The VPN test is Windows-only and hasn't been built.

### `ui_public_dependencies` → direct deps

Upstream finished removing `browser_public_dependencies` and its `browser/ui`
counterpart, so targets list their direct deps
([1d4b690ab57b](https://chromium.googlesource.com/chromium/src/+/1d4b690ab57b511ef64f9bb681aef7127036573a)).
The rebase routed Brave's UI deps through a new group with the same role
("[cr158] Route the Brave UI public deps through `browser`").

- [x] `browser/ui/BUILD.gn:39` `ui_public_dependencies` and
      `browser/sources.gni:799` `brave_chrome_browser_public_deps`: move the
      entries to the targets that use them, and verify with `gn check`
- [ ] Run `gn check` for Android, macOS and Windows; only Linux was checked

Done in "[cr158] Replace the Brave public deps of `browser` with direct deps".

### Pre-existing: `chromium_src` overrides include headers their targets don't depend on

`gn check` reads the upstream file a target lists, not the `chromium_src`
override compiled in its place, so these includes are unchecked. In these 11
upstream targets the override includes a Brave header with no dependency path
from the target to its owner (Linux, `gn path`). Generated headers then only
exist at compile time by build order, and none of these targets depend on
`//chrome/browser`, so its old public deps never covered them.

| Target                                                          | Needs                                |
| --------------------------------------------------------------- | ------------------------------------ |
| `//chrome/browser/component_updater:impl`                       | `//brave/browser:browser_process`    |
| `//chrome/browser/download:impl`                                | `//brave/browser:browser_process`    |
| `//chrome/browser/renderer_context_menu:impl`                   | `//brave/browser:browser_process`    |
| `//chrome/browser/ssl:impl`                                     | `//brave/browser:browser_process`    |
| `//chrome/browser/prefs:impl`                                   | `//brave/browser/ui:brave_tab_prefs` |
| `//chrome/browser/ui:layout_constants`                          | `//brave/browser/ui:brave_tab_prefs` |
| `//chrome/browser/ui/views/frame:browser_root_view_impl`        | `//brave/browser/ui:brave_tab_prefs` |
| `//chrome/browser/ui/views/tabs/hovercard:impl`                 | `//brave/browser/ui:brave_tab_prefs` |
| `//chrome/browser/ui/views/tabs:base`                           | `//brave/browser/ui:brave_tab_prefs` |
| `//chrome/browser/profiles:profile_util_impl`                   | `//brave/app/vector_icons`           |
| `//chrome/browser/ui/views/profiles:avatar_toolbar_button_impl` | `//brave/app/vector_icons`           |

- [ ] Move the Brave code behind forward-declared hooks implemented in Brave
      targets linked through `//brave/browser:core`, per
      docs/best-practices/chromium-src-overrides.md (CSRC-019), rather than
      patching Brave deps into these targets

### WebUI accelerators through `SetAcceleratorHandling()`

[533628dca8b93](https://chromium.googlesource.com/chromium/src/+/533628dca8b930d5a0c9b14d2aad64e1dda83399)
added `views::WebContentsDelegateViews` and deprecated
`WebView::set_allow_accelerators()`; 14 follow-ups under Bug 573113131 migrate
upstream's WebUI surfaces (e.g.
[0661d0d0f7cdf](https://chromium.googlesource.com/chromium/src/+/0661d0d0f7cdfb4c843fb4e29a9f710e45fd94a0):
a `HandleKeyboardEvent` override and its `UnhandledKeyboardEventHandler` member
become
`SetAcceleratorHandling(views::WebContentsAcceleratorHandling::kEnabled)`).
Upstream files with `UnhandledKeyboardEventHandler` fell from 38 to 19
(`git grep -l 'UnhandledKeyboardEventHandler' <tag> -- '*.cc' '*.h' | wc -l`).
All paths are under `browser/ui/views/`:

- [ ] `page_info/brave_shields_page_info_view.cc:83-98` (`ShieldsWebView`)
- [ ] `side_panel/ai_chat/ai_chat_movable_side_panel_web_view.h:113,151` and
      `.cc:251-256`
- [ ] `brave_origin/brave_origin_startup_view.h:150,166` and `.cc:401-405`: a
      `WidgetDelegate` and `WebContentsDelegate`, so it becomes a
      `WebContentsDelegateViews<…>`
- [ ] `speedreader/reader_mode_toolbar_view.cc:40`: the rebase moved it to
      `set_allow_accelerators_deprecated(true)`, which is for backwards
      compatibility only (crbug 576327952); move it to `kEnabled` or
      `kProcessBeforeRenderer`

### iOS: `GetForProfile(ProfileIOS&)`

Typed iOS factories gained reference overloads of `GetForProfile` and
`GetForProfileIfExists`
([c352f5d3308ce](https://chromium.googlesource.com/chromium/src/+/c352f5d3308ce82275bb6d64698185f7afd4c143)),
and a `BuildServiceInstanceFor(ProfileIOS&)` that the pointer form forwards to
until a factory converts
([b7555dcf2eb9f](https://chromium.googlesource.com/chromium/src/+/b7555dcf2eb9f5c4927c3504ae8e32cefcaf82df),
Bug 576182342). Only the preparation landed: upstream `GetForProfile(*` in
`ios/` went from 0 to 4 (`git grep -cE 'GetForProfile\(\*' <tag> -- ios`), and
the pointer form isn't deprecated. Brave has about 115 iOS `GetForProfile(`
calls.

- [ ] When upstream factories start overriding the reference
      `BuildServiceInstanceFor`, move Brave's typed factories (from
      "[cr158][ios] Convert factories to `TypedProfileKeyedServiceFactoryIOS`")
      to it

## 4. Obsolete Brave code

### Brave's variations 304 country hook duplicates upstream

Upstream now reads `X-Country` and `X-Geo-Level-1` on 304 seed responses, and
`RecordSuccessfulFetchSeedNotModified()` calls
`UpdateSessionCountryAndGeoLevel1()`, which also updates the in-memory seed
cache
([34f26b3fbf0f0](https://chromium.googlesource.com/chromium/src/+/34f26b3fbf0f00206c2ffdb217a63a1ca2f72c17)).
That is what Brave's hook does (master 02ad2c9717a and 25059f6aafe), minus
upstream's refusal to trust headers on an insecure HTTP retry.

- [ ] `chromium_src/components/variations/service/variations_service.cc:8-27`:
      drop the `GetDateValue` hook, along with `SetSessionCountry` in
      `chromium_src/components/variations/seed_reader_writer.cc:12-18` and
      `variations_seed_store.cc:36-38`, their plasters
      (`rewrite/components/variations/{seed_reader_writer,variations_seed_store}.h.yaml`)
      and unit tests. First confirm that Brave's variations server sends
      `X-Geo-Level-1`, or that an empty one is fine: upstream overwrites it
      whenever the country changes

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
warning the filter comment cites. In 8115.1..8117.1,
[845d377ebe5d](https://chromium.googlesource.com/chromium/src/+/845d377ebe5dc8e48537ff830fcc60e85585b072)
deleted the Related Website Sets suites, and the signin flag clean-up
(Bug 411329731) deleted `turn_sync_on_helper_browsertest.cc`
([bd29e0fac755](https://chromium.googlesource.com/chromium/src/+/bd29e0fac7556de0f99e1022fa945d80f702b5e1))
and `DiceBrowserTestWithoutReplaceSyncPromosWithSignInPromos`
([1b9ece651400](https://chromium.googlesource.com/chromium/src/+/1b9ece6514006ea60278bbca003ddcffe7d98573)).

- [ ] `test/filters/browser_tests.filter:920` and `:1621-1625`
      (`RequestStorageAccessFor*`)
- [ ] `test/filters/browser_tests.filter:2046`
      (`TopLevelStorageExemptionReasonMetricTest`, with its comment)
- [ ] `test/filters/browser_tests.filter:440`
      (`ErrorConsoleBrowserTest.BrowserActionRuntimeError`, now
      `ActionRuntimeError`) and `:1234` (`PromoBubbleAccept_OpensDiceSignInTab`)
- [ ] `test/filters/browser_tests.filter:94`
      (`OmniboxAimPopupDeferCreationPendingBrowserTest.LocationBarActionCreatesAimContent`,
      deleted by
      [8060af6aee203](https://chromium.googlesource.com/chromium/src/+/8060af6aee2032c615a7be3cfb92120b54980a74))
- [ ] `test/filters/unit_tests.filter:215` (`MaterialTabStripColorMixerTest.*`)
- [ ] Related Website Sets: `test/filters/unit_tests.filter:228-229`, `:236-238`
      (with the comment), `:243-244` and `:582-583`, and
      `test/filters/browser_tests.filter:928-929` and `:3360`. At 8127.2, also
      `unit_tests.filter:230`
      (`PageInfoRelatedWebsiteSetsTest.ShowAutograntedRWSPermissions`,
      [adf604b51ed62](https://chromium.googlesource.com/chromium/src/+/adf604b51ed625eaa4f8d5e165efad3e365dcfe2))
- [ ] Signin: `test/filters/browser_tests.filter:859-863`, `:1809-1811`
      (`TurnSyncOnHelperBrowserTest*`) and `:1218`
      (`DiceBrowserTestWithoutReplaceSyncPromosWithSignInPromos.*`). At 8127.2,
      also `:3235`
      (`ProfilePickerCreationFlowBrowserTest.SyncConfirmationNavigateBackTest`,
      [6e233a559e7d6](https://chromium.googlesource.com/chromium/src/+/6e233a559e7d60753d9ef886f6cefa5bfc695d9f))
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
      [dd9b3c1acc142](https://chromium.googlesource.com/chromium/src/+/dd9b3c1acc1423c6d8de2f8adb0fdbb84b1057af).
      At 8117.1, also
      `PrintPreviewScalingSettingsTest.InputNotDisabledOnValidityChange`
      (`browser_tests-linux.filter:3921`, `browser_tests-win.filter:4296`),
      moved to interactive_ui_tests
      ([51220aea6e2b](https://chromium.googlesource.com/chromium/src/+/51220aea6e2b7f9276f6e90d399d0e9cd510ab59)),
      `SystemNetworkContextManagerWithFirstPartySetComponentBrowserTest`
      (`browser_tests-win.filter:4767`),
      `HistoryClustersConfigTest.LocaleOrLanguage*`
      (`components_unittests-linux-asan.filter:66,69`,
      [6ebcee1b6615](https://chromium.googlesource.com/chromium/src/+/6ebcee1b6615b7e29c965960716554874aa7c7da)),
      `HatsHandlerTest.*SecurityPage*V2*`
      (`unit_tests-linux-asan.filter:318,321`,
      [add1f2b27660](https://chromium.googlesource.com/chromium/src/+/add1f2b276601423187b9ad56cdcf67fa96754f7)),
      and `DictationGlicBrowserTest.BasicStreamFunctions`
      (`browser_tests-linux-msan.filter:636`), which became `All/…/*`
      ([eba365f6e996](https://chromium.googlesource.com/chromium/src/+/eba365f6e9969ee770a91f9080927edc22fe18fb)).
      At 8127.2, also `All/PDFExtensionJSInk2Test.Ink2SizeSelector/1`
      (`browser_tests-linux-asan.filter:282`) and
      `PDFExtensionJSInk2BeforeUnloadTest.Stroke`
      (`browser_tests-win.filter:4209`), renamed to `Ink2DrawSizeSelector` and
      `DrawStroke`
      ([ab1aff277bbb1](https://chromium.googlesource.com/chromium/src/+/ab1aff277bbb1bea5aa81d1b1486289ffcc569b5)),
      and the ChromeOS-only `KioskEnrollmentTestWithAddUserFlowEnabled` entry
      (`browser_tests-linux-msan.filter:948`,
      [773d7fd1085c9](https://chromium.googlesource.com/chromium/src/+/773d7fd1085c950bf5bbcdb8b628d90bba3e3052))

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
- [ ] `test/filters/browser_tests.filter:3097,3099`: the old
      `IndigoHighDsfBrowserTest.*` and `IndigoOnboardingBrowserTest.*` entries
      match nothing now that `:746-747` cover the `All/…` suites; drop them
- [x] `test/filters/browser_tests.filter:1745`:
      `ExtensionOpenSidePanelViewsToolbarBrowserTest.PinExtensionViaSidePanelJourneyCompletion`
      ([301a9d992e895](https://chromium.googlesource.com/chromium/src/+/301a9d992e895117f69cb54a48ff21a6787f2cc3))
- [x] `test/filters/browser_tests.filter:1963`:
      `TtcSessionJournalImplBrowserTest.JournalFollowsTheSessionTask`
      ([70ef0900441d9](https://chromium.googlesource.com/chromium/src/+/70ef0900441d902273eb3326fe9a3e861fdc4697)).
      This and the one above were fixed by "[cr158] Filter upstream tests as
      needed"
- [ ] `test/filters/browser_tests.filter:1399`: the suite is now
      `SendTabToSelfBubbleControllerBrowserTest.ScrollPositionPropagated_ScrolledPage`
      ([65d99b0dbd18e](https://chromium.googlesource.com/chromium/src/+/65d99b0dbd18ef70fc023a727bae6f7a4afcd4b9))
- [ ] `test/filters/components_unittests.filter:293-295`:
      `GetPreferredAccountForPromo*` → `GetPreferredAccountForSignin*`
      ([69060d1c866bc](https://chromium.googlesource.com/chromium/src/+/69060d1c866bca1bcd1784e0b9c1bf43626f94c1))
- [ ] `test/filters/unit_tests.filter:25`:
      `All/CanOfferSigninWithSigninToSyncFeatureTest.*` was folded into
      `CanOfferSigninTest.OtherProfileAlready*`
      ([bd29e0fac755](https://chromium.googlesource.com/chromium/src/+/bd29e0fac7556de0f99e1022fa945d80f702b5e1)),
      which now runs; retarget it
- [x] `test/filters/browser_tests.filter:1309`:
      `ForceSigninProfilePickerCreationFlowBrowserTest` became `_F`
      ([7ffe36f6c57b](https://chromium.googlesource.com/chromium/src/+/7ffe36f6c57b0c0d9d833dd79b9a2274f16293a8));
      the `All/` prefix is gone, fixed by "[cr158] Filter upstream tests as
      needed"
- [ ] `test/filters/browser_tests.filter:875`: now
      `SigninUiUtilExtensionsTest.ShowExtensionSigninPrompt`
      ([1b9ece651400](https://chromium.googlesource.com/chromium/src/+/1b9ece6514006ea60278bbca003ddcffe7d98573)).
      "[cr158] Filter upstream tests as needed" added the new name at `:876`;
      drop the old `All/…` entry
- [ ] `test/filters/net_unittests.filter:24-25,61,63-65`: the key pinning tests
      now also run as
      `KeyPinningStateChrome.{PinValidationWithCompiledInPins,     PreloadedProductionStaticPins,UpdateKeyPinsEmptyList,     UpdateKeyPinsListTimestamp,UpdateKeyPinsListValidPin}`
      (`net/cert/key_pinning_state_chrome_unittest.cc`,
      [5691202e21434](https://chromium.googlesource.com/chromium/src/+/5691202e214341a5d96a3296c1b9811cce19572b)),
      unfiltered; add them (confirm on a `net_unittests` run), and drop the
      duplicate `PreloadedProductionStaticPins` at `:61`
- [ ] `test/filters/browser_tests.filter:1744`: the comment names `kJourneys`,
      which upstream removed; Brave now disables Journeys through `Config`
      ("[cr158] Ship Journeys disabled via Config instead of the removed
      kJourneys")

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
- [ ] Pre-existing bytecode adapter operations whose target is gone at both
      tags, under `build/android/bytecode/java/org/brave/bytecode/`:
      `BraveToolbarManagerClassAdapter.java:66-67`
      (`mOverlayPanelVisibilitySupplier`),
      `BraveBookmarkUtilsClassAdapter.java:31` (`isSpecialFolder`, now an
      instance method of `BookmarkModel`),
      `BraveTabbedActivityClassAdapter.java:43` (`supportsDynamicColors`),
      `BraveToolbarLayoutClassAdapter.java:26`
      (`ToolbarPhone.onHomeButtonUpdate`) and
      `BraveBookmarkToolbarClassAdapter.java:20-21`
      (`BookmarkToolbar.mBookmarkModel`). Drop them with any `BytecodeTest`
      assertions behind them
- [ ] `android/junit/src/org/chromium/chrome/browser/ntp/BraveRecentTabsCoordinatorTest.java`:
      pre-existing since "[cr157][Android] Drop Recent tabs refresh workaround"
      (master 2907b401839), which removed its GN entry and the plaster it
      tested. It is in no target, and the classes it uses moved to `recent_tabs`
      in this range. Delete it
- [x] "[WIP][cr158][Android] Drop Nala icons orphaned by the main settings
      cleanup": `ic_location_on` was not orphaned — Brave's own cr157
      `11c40e86506` rewrites the contact-info row to it, and `nala_icons` is its
      only provider, so dropping the entry broke
      `brave_junit_tests__compile_resources`; restored. `ic_credit_card` stays
      dropped: the payments row keeps the Nala icon through the `credit_card`
      override, which reads the Leo source directly, confirmed on device. WIP
      tag removed
- [ ] `base/compile_overridden_features.inc`: pre-existing names that aren't
      upstream features at either tag: `:38` `CommerceDeveloper`, `:62`
      `EnableForceDownloadToOneDrive`, `:138` `PlusAddressesEnabled`, and `:179`
      `VerticalTabsLaunch` (only a What's New module name). At 8127.2, also
      `:165` `ShowRelatedWebsiteSetsPermissionGrants`
      ([adf604b51ed62](https://chromium.googlesource.com/chromium/src/+/adf604b51ed625eaa4f8d5e165efad3e365dcfe2)),
      which "[cr158] Drop kShowRelatedWebsiteSetsPermissionGrants override" left
      behind
- [ ] `chromium_src/chrome/common/url_constants.h`: this replaces the upstream
      header outright, and still defines `kLegacySyncGoogleDashboardURL` and
      `kNewSyncGoogleDashboardURL` (`:507-513`), which upstream merged into
      `kSyncGoogleDashboardURL`
      ([f00039ee79aa0](https://chromium.googlesource.com/chromium/src/+/f00039ee79aa0273cfb9bf15272d1b971740c571)).
      Pre-existing: 24 more constants exist in no upstream `url_constants.h` at
      8127.2 and nothing in Brave uses them (e.g.
      `kFlashDeprecationLearnMoreURL`, `kPaymentMethodsURL`). Upstream now
      defines `kExtensionInvalidRequestURL` and `kIsolatedAppScheme[Utf16]` in
      other headers, so check those before dropping. List them by running
      `comm -23` over the `constexpr char(16_t)? k\w+` names of both files
- [ ] `chromium_src/content/browser/browser_context.cc:31-60` and
      `chromium_src/content/public/browser/browser_context.h:33-39`:
      pre-existing, since master 9877df40bbb nothing calls
      `CreateSessionStorageNamespace()` or `GetSessionStorageNamespaceId()`.
      Session storage now rejects namespace IDs that aren't UUIDs
      ([ac19329a1d321](https://chromium.googlesource.com/chromium/src/+/ac19329a1d32108fd05ead15568cb6e188f8615a),
      [02dc3d4c096c8](https://chromium.googlesource.com/chromium/src/+/02dc3d4c096c811152ac22a557941aeceb8ed4fc)),
      so a new caller would hit `ReportBadMessage`. Delete them
- [ ] `chromium_src/components/component_updater/component_installer.cc:46`:
      nothing registers the First Party Sets component
      (`gonpemdgkjcecdgbnaabipppbmgfggbe`) since
      [845d377ebe5d](https://chromium.googlesource.com/chromium/src/+/845d377ebe5dc8e48537ff830fcc60e85585b072);
      drop the entry, or keep it as a pin while the installer policy remains

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
Since 8127.2 the iOS `UniversalOptOutServiceFactory` creates the service from
`kUniversalOptOut` alone
([0935757d85abf](https://chromium.googlesource.com/chromium/src/+/0935757d85abf3d64f9603afa7548c7b07785b1f)),
so pinning that flag also stops the service on iOS.

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

[d744b45d1485](https://chromium.googlesource.com/chromium/src/+/d744b45d148585827d888b02747317094177e877)
reverted the clean-up of the "Tabs from other devices" side panel, which
upstream now plans to launch. Under `features::kTabsFromOtherDevicesSidePanel`
(off) it adds `IDC_SHOW_TABS_FROM_OTHER_DEVICES_SIDE_PANEL` to
`RecentTabsSubMenuModel::Build()`, a WebUI side panel and a toolbar action, all
bypassing Brave's synced-tabs override in
`chromium_src/chrome/browser/ui/tabs/recent_tabs_sub_menu_model.cc`.

- [ ] `rewrite/chrome/browser/ui/ui_features.cc.yaml`: pin
      `kTabsFromOtherDevicesSidePanel` and
      `kTabsFromOtherDevicesSidePanelPinnedByDefault` disabled

`ntp_features::kNtpComponentExtension` (off) loads a component extension whose
manifest overrides `chrome://newtab`
([d58babee2acb6](https://chromium.googlesource.com/chromium/src/+/d58babee2acb62c4e9323743b929b05e74921f7f)).
`BraveComponentLoader` calls through to upstream's loader and the extension is
allowlisted, so it would replace Brave's NTP if the flag flips. Since 8117.1 it
is also downloaded through the component updater, under the
`use_component_updater` param (default true), as
`pkohanfffhahpakjbppgjidngeadjcfj`
([51602f3c1839](https://chromium.googlesource.com/chromium/src/+/51602f3c18392d7fe76d12f22c23040146bd6270)).

- [ ] `rewrite/components/search/ntp_features.cc.yaml`: pin
      `kNtpComponentExtension` disabled, and add it to
      `app/feature_defaults_unittest.cc`; optionally deny
      `extension_misc::kNewTabPageExtensionId` in
      `chromium_src/chrome/browser/extensions/component_extensions_allowlist/allowlist.cc:34`
- [ ] `chromium_src/components/component_updater/component_installer.cc:38`: add
      `pkohanfffhahpakjbppgjidngeadjcfj` to `kDisallowedComponents`, and the AIM
      eligibility component (`kabpfonokkamggpgbldambnmkliehlbh`, behind
      `kAimEligibilityComponentExtension`, off)

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
Both are off today. At 8127.2, `kContextualTasksRearchitecture` also adds
`ContextualTasksURLLoaderThrottle` and proxies Google subresource fetches to add
a `Chrome-Search-Capabilities-Version` header, from `WillCreateURLLoaderFactory`
([c220cfad1c02b](https://chromium.googlesource.com/chromium/src/+/c220cfad1c02b300858c0d8de003f1a9ab54fe97)).
`IsContextualTasksUIEnabled()` is already on through
`kContextualTasksSidePanel`; see "Contextual Tasks turns on past Brave's
`kContextualTasks` pin".

- [ ] Pin `kContextualTasksSidePanelRearchitecture` and
      `kContextualTasksRearchitecture` disabled

`features::kTopChromeWebUISharedRenderer` (off) creates
`TopChromeWebUIProfileImpl`, a new `BrowserProfileType::kTopChromeWebUIProfile`
that is off-the-record, its own original profile, and has no keyed services
([f28aa18be961](https://chromium.googlesource.com/chromium/src/+/f28aa18be961c4deafc774f7fa10b611dc8f71c8)).
Follow-up CLs will host Top Chrome WebUIs in it. Brave's
`DefaultTopChromeWebUIConfig` UIs (AI Chat, Wallet, Shields, VPN, Playlist,
Speedreader, Rewards and News panels, tab search, bookmarks) would then get a
profile without Brave's prefs and services, and Brave's
`IsPrimaryOTRProfile`/`BrowserProfileType` checks don't know the type.

- [ ] Pin `kTopChromeWebUISharedRenderer`
      (`chrome/common/chrome_features.cc:2009`) disabled, and add it to
      `app/feature_defaults_unittest.cc`

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
      `chromium_src/chrome/browser/resources/settings/autofill_page/autofill_page_index.html.ts.lit_mangler.ts:30`

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

### Nothing tests that Journeys stays off

"[cr158] Ship Journeys disabled via Config instead of the removed kJourneys"
replaces the `kJourneys` pin
([6ebcee1b6615](https://chromium.googlesource.com/chromium/src/+/6ebcee1b6615b7e29c965960716554874aa7c7da)
hard-codes Journeys on for desktop). Both non-test readers of
`is_journeys_enabled_no_locale_check` go through `GetConfig()`, so the plaster
covers them, but the commit dropped `kJourneys` from
`app/feature_defaults_unittest.cc`.

- [ ] Add a test that
      `history_clusters::GetConfig().is_journeys_enabled_no_locale_check` is
      false

### Android: JNI native pointers as `JniUniquePtr` / `JniRawPtr`

36 commits under Bug 40794873 replace `long mNativeX` with
`JniUniquePtr<NativeX>` (owning) or `JniRawPtr<NativeX>`, and
`reinterpret_cast<intptr_t>(this)` with `jni_zero::MakeUnique/MakeRaw(this)`
(e.g.
[c16e504f0fde](https://chromium.googlesource.com/chromium/src/+/c16e504f0fde7cb85f23e19f5f8331b907b9bdfa)).
Java files using `JniUniquePtr<` rose from 16 to 38 in 8115.1..8117.1, and
`JniRawPtr<` from 9 to 31. Brave has 12
(`git grep -nE 'private (final )?long mNative\w+' -- '*.java'`).

- [x] The backported `browser/password_entry_edit/.../CredentialEditBridge.java`
      and `browser/password_manager/.../PasswordUiView.java` migrated to
      `JniRawPtr`. Pre-existing on master, so fixed there:
      [#60325](https://github.com/brave/brave-browser/issues/60325), fixed by
      [#41102](https://github.com/brave/brave-core/pull/41102). `JniRawPtr` and
      not `JniUniquePtr` because `PasswordUiViewAndroid::Destroy()` sets
      `DELETION_PENDING` instead of deleting while an export task holds
      `base::Unretained(this)`, finishing in `PostSerializedPasswords()` — the
      owning handle's glue would free at the Java boundary. Note they cannot
      "follow upstream's originals": upstream deleted both components
      (`6875d2ce472e7`, `3662471ee9fab`) about a year before the campaign began
      (`ff47b0edea856`), and their last upstream forms still used a raw `long`.
      The pattern copied is
      [fcd2c8c09a727](https://chromium.googlesource.com/chromium/src/+/fcd2c8c09a72752233214d440d159ee39e122d57)
- [x] The other 10 are Brave-only classes with Brave-owned native counterparts,
      so no upstream CL will ever migrate them. Also master debt, and each needs
      its own ownership decision plus a device test plan, so one issue each with
      the analysis in the body; PRs follow the rest of M158:
      [#60327](https://github.com/brave/brave-browser/issues/60327)
      `BraveSyncWorker`,
      [#60328](https://github.com/brave/brave-browser/issues/60328)
      `BraveSyncDevices`,
      [#60329](https://github.com/brave/brave-browser/issues/60329)
      `BraveVpnNativeWorker`,
      [#60330](https://github.com/brave/brave-browser/issues/60330)
      `BraveRewardsNativeWorker`,
      [#60331](https://github.com/brave/brave-browser/issues/60331)
      `BraveShieldsContentSettings`,
      [#60332](https://github.com/brave/brave-browser/issues/60332)
      `BraveLeoMojomHelper`,
      [#60333](https://github.com/brave/brave-browser/issues/60333)
      `NTPBackgroundImagesBridge`,
      [#60334](https://github.com/brave/brave-browser/issues/60334)
      `BraveReferrer`,
      [#60335](https://github.com/brave/brave-browser/issues/60335)
      `BraveDappPermissionPromptDialog`,
      [#60336](https://github.com/brave/brave-browser/issues/60336)
      `BraveBookmarkBridge`. The last is not a safe-pointer fix: its field is
      deleted by `BraveBookmarkBridgeClassAdapter` and upstream's is made
      protected, so Brave cannot type it — filed as an investigation into
      replacing that adapter with a plaster first. Four of the others
      (`BraveSyncWorker`, `BraveSyncDevices`, `BraveVpnNativeWorker`,
      `BraveRewardsNativeWorker`) share one shape: void `Init` plus a `private`
      `setNativePtr(long)` callback, which `JniUniquePtr` removes outright, also
      satisfying jni_zero's package-private requirement on `@CalledByNative`
      safe-pointer parameters

### Script the Android names-in-strings check

Resolving every `BraveReflectionUtil` literal, `BytecodeTest` string and
bytecode adapter target (with parameter lists) against the upstream class at the
new tag found items in M157, 8104.1 and 8107.2.

- [ ] Add a `tools/cr/` script that does this for two tags. Not lift work and
      not specific to a pin, so filed for master with the design in the body:
      [#60337](https://github.com/brave/brave-browser/issues/60337), and
      requested under [Tooling TODOs](#tooling-todos). Resolving from source at
      each tag (a `git ls-tree` index, then `git show`) rather than from
      bytecode, because the bytecode ground truth would need an Android build at
      the _old_ tag. Two tags and not one so that "broke between tags" separates
      from pre-existing debt. Note none of the three families fails loudly
      today: `BraveReflectionUtil` returns null in release builds
      (`BraveReflectionUtil.java:57`), `BytecodeTest` is an on-device javatest,
      and a stale adapter target simply never matches its map entry
      (`BraveClassVisitor.java:178,234,255`)

### License metadata uses SPDX identifiers

Upstream normalised `SPECIAL_CASES` licenses
([ebe50178d4754](https://chromium.googlesource.com/chromium/src/+/ebe50178d4754320c29492023f994de49dcae68b)).

- [ ] `script/brave_license_helper.py:319`: `"BSD"` → `"BSD-3-Clause"`

### Android: "Report an issue" in the Help submenu

Upstream re-added "Report an issue" under `help_parent_menu_id`
([cd627cf679c4e](https://chromium.googlesource.com/chromium/src/+/cd627cf679c4e1089c9712c53457b6d3f5737f8d)).
Brave removes only `help_id`, and doesn't pin `kSubmenusInAppMenu` or
`kSubmenusInAppMenuLff`, both off by default. Predates the range.

- [ ] `android/java/org/chromium/chrome/browser/tabbed_mode/BraveTabbedAppMenuPropertiesDelegate.java:1044`:
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
      `android/javatests/org/chromium/chrome/browser/BytecodeTest.java:974-985`:
      redirect only the descriptor Brave overrides

### Android: upstream's Play in-app update flow

[f742d8b3cd709](https://chromium.googlesource.com/chromium/src/+/f742d8b3cd709)
adds `InAppUpdateController` behind `kInAppUpdateFlow` (off). With the flag off,
the app menu "Update" item keeps its old intent fallback.

- [ ] `android/java/org/chromium/chrome/browser/app/BraveActivity.java:443-460,1711-1790`:
      if the flag launches, adopt upstream's flow instead of Brave's own

### Android: dead null check in `BraveExternalNavigationHandler`

[543e673b21d3](https://chromium.googlesource.com/chromium/src/+/543e673b21d3d65611e526f308562626d08aa824)
made the target URL argument non-null. The check was live before it: the
two-argument `startActivity()` overload passed a literal null, reaching Brave's
override from seven internal call sites. The CL changed that argument to
`GURL.emptyGURL()` and routed the other caller through the new
`getTargetUrlFromIntent()`, which is non-null on both paths — dropping
`@Nullable` only documented the result. Behaviour is unchanged either way, since
`GURL::DomainIs()` returns false for an invalid URL.

- [x] `android/java/org/chromium/chrome/browser/externalnav/BraveExternalNavigationHandler.java:56`:
      drop `intentTargetUrl != null` — done
- [x] marked the class `@NullMarked`, so the override's nullability is actually
      checked; needs the three upstream-nullable `startActivity()` parameters
      annotated to match the supermethod

### Android: WebUI history's "Delete browsing data" opens Quick Delete

`BrowsingHistoryHandler::OpenClearBrowsingDataDialog()` on Android now opens
upstream's Quick Delete dialog instead of the Clear Browsing Data screen
([23765c0be85af](https://chromium.googlesource.com/chromium/src/+/23765c0be85afdbfcddb6012c0ec26844bcc37f8)).
Brave removes Quick Delete from the app menu
(`BraveTabbedAppMenuPropertiesDelegate.java:1041-1042`). The path runs only with
Android WebUI history, behind `kAndroidDesktopWebUiHistory` (off, unpinned).

- [ ] Before that flag launches, keep the Android branch on Brave's Clear
      Browsing Data screen (a plaster on `browsing_history_handler.cc`, which
      Brave already plasters for the sync promo), or pin the flag off

### `IsChromeUntrustedDataSource` copy builds a static string

Upstream's original stopped building a static `NoDestructor<std::string>`
([ace9b5d874d9b](https://chromium.googlesource.com/chromium/src/+/ace9b5d874d9bdd0651ffa2b42867509ea9f7bbf)).

- [ ] `chromium_src/ui/webui/webui_util.cc:21-27`: use upstream's
      `constexpr std::string_view` check

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
- `kThreeDSecureTelemetry` is on: `WebPaymentsObserver`, in every tab's
  `TabFeatures`, parses POST form bodies on submission and records 3DS UMA/UKM
  ([36ac4be459c5](https://chromium.googlesource.com/chromium/src/+/36ac4be459c52942566731f4daf6df6bff8cc21c)).
- Desktop HaTS surveys carry the channel and version
  (`kHappinessTrackingSurveysDefaultProductSpecificData`, on); Brave's plaster
  still blocks every launch
  ([e9d5d887188d](https://chromium.googlesource.com/chromium/src/+/e9d5d887188d9c90dfaf314895a949ed1bff0ddc)).
- Android: the NTP "plus" (Fusebox) menu is on by default, gated by a Google
  default search engine and Brave's `kAimEnabled` pin
  ([4dd2354d3b6e](https://chromium.googlesource.com/chromium/src/+/4dd2354d3b6edefdc445dfa7701ea01cf53b2ce0)).
- Device Bound Session Credentials JS API, behind the experimental
  `DeviceBoundSessionJsApi` Blink feature
  ([38e23936ced2](https://chromium.googlesource.com/chromium/src/+/38e23936ced21293ac47a9a44fb2de0669c716fb)).
- Cross-device sign-in QR bubble from the What's New page,
  `kCrossDeviceSigninFromDesktopFollowup` (off)
  ([566635fe3ce2](https://chromium.googlesource.com/chromium/src/+/566635fe3ce2912169a6ca76a00a7ee1d8e400e9)).
- Context Hub skill recording service, `kRecordReplay` (off)
  ([e0d30d508324](https://chromium.googlesource.com/chromium/src/+/e0d30d508324a3b6e6855bd089f0316b9df7d727)).
- Android: an intranet-redirect prompt after a single-word omnibox HEAD probe
  ([2d6abd1080a8](https://chromium.googlesource.com/chromium/src/+/2d6abd1080a8bcf040da05bd97725cd00b540f38)).
- Related Website Sets were removed and `first_party_sets.db` is deleted on
  startup
  ([845d377ebe5d](https://chromium.googlesource.com/chromium/src/+/845d377ebe5dc8e48537ff830fcc60e85585b072)).
- `kContextualTasksSidePanel` is on on desktop (see "Contextual Tasks turns on
  past Brave's `kContextualTasks` pin").
- `kNtpRealboxDynamicAiModeButton` and `kWebUIOmniboxDynamicAiModeButton` are
  on: an animated AI Mode button in the realbox and WebUI omnibox
  ([81231996908f](https://chromium.googlesource.com/chromium/src/+/81231996908f9e6285ea612a3ac0f1d5f4c4a104));
  Brave's NTP has no realbox, and the WebUI omnibox is off.
- iOS: `kIOSPasswordAutoSubmission` is on
  ([7ea0e4ba65015](https://chromium.googlesource.com/chromium/src/+/7ea0e4ba65015)).
- `WEBGL_shader_pixel_local_storage` is exposed in `getSupportedExtensions()`
  ([6cbd1f29e756](https://chromium.googlesource.com/chromium/src/+/6cbd1f29e756)),
  one more fingerprinting bit.
- New flags, all off: `kGlicActivityHub`, `kGeicActuation`, `kGeicFre`,
  `kDynamicAimFulfillment{,Omnibox,Realbox}`, `kRustPhishingModelSetter`,
  `kEnterpriseRealTimeUrlCheckUrlSkiplist`.

## Tooling TODOs

Requests to improve the rebase and lift tooling (🚀Brockit, 🩹Plaster, `git cr`,
`pnpm run` commands, `tools/recipes`, `tools/cr/autolift`), so that less of the
lift depends on judgement. Filed by lift runs; see
[SKILL.md](autolift/SKILL.md#tooling).

### `brockit rebase --continue`: resume and resolve the mechanical conflicts

`brockit rebase --squash-minor-bumps` stops on every conflict and hands over to
plain `git rebase --continue`, so each stop is manual, and Brockit's own
handling (squash planning, the final report) doesn't resume. Most stops have a
fixed answer. In the 2026-10-29 rebase of cr158 onto `origin/cr157` at
`157.0.8103.30`, the run stopped seven times; all but two (the fixup ordering
bug below, and a test-filter merge) had a fixed answer.

Landed in "[brockit] Add `rebase --continue`" (#41071), which also adds
`--skip`: the rebase resolves these conflicts as it goes, stops only on a
conflict outside them, listing the commit and files left, and reports the
resolutions once it finishes.

- [x] Add `--continue` to `brockit rebase`, resuming Brockit's own rebase rather
      than leaving the rest to git
- [x] Conflicted `patches/*.patch` with a yaml: rerun its plaster
      (`plaster apply <patch>`), unless the yaml itself is conflicted
- [x] `tools/cr/M<N>_codehealth.md`: a tracker the base doesn't have takes the
      branch's last version
- [x] The "Update from Chromium" commit, when the base moved to a newer minor
      version: in `package.json`, take the commit's `projects.chrome.tag`, and
      in
      `chromium_src/net/tools/transport_security_state_generator/input_file_parsers.cc`,
      take the newer of the two `PinsListTimestamp` values and its "Last
      updated" comment. Do it only when that is the file's single conflicting
      hunk
- [x] Raw patches (no yaml): when the base changed only context lines, not the
      patch's own `+`/`-` lines, take the commit's version (cr158:
      `chrome-browser-ui-views-frame-browser_view.cc.patch` and
      `chrome-browser-ui-views-location_bar-location_bar_view.cc.patch`). List
      each such patch as a candidate for conversion to plaster
- [x] Choose the base itself (`origin/cr<N-1>` until master's Chromium tag is on
      milestone `N-1`, then `origin/master`), or warn when the branch's upstream
      doesn't match it. Closed: `brockit rebase` keeps requiring the upstream to
      be set beforehand, as an explicit base is less confusing;
      `autolift.py prepare` sets it

### `brockit rebase`: place fixups after their pinned group's last commit

Squash planning puts a `fixup!` of a pinned group right after the group's first
commit. If a later commit of the group moved the files the fixup touches, the
fixup fails with a modify/delete conflict, and git's rename detection may apply
it to the old path instead. cr158: a fixup making
`tools/cr/autolift/autolift.py` executable came before the group's commit that
renamed `tools/cr/lift/` to `tools/cr/autolift/`. The workaround is to keep
HEAD's state for that step and requeue the fixup after the group's last `squash`
in `git-rebase-todo`.

- [ ] Order a pinned group's fixups after its last member, with a test in
      `rebase_test.py`

### `plaster`: `preempt_function_impl` under-indents in-class definitions

The block is documented to be indented to the body's first-statement level, but
in a method defined inside its class it gets the top-level indent: two spaces
instead of four (cr158:
`BrowserView::ExclusiveAccessContextImpl::UpdateExclusiveAccessBubble` in
`browser_view.cc`, which keeps the rewriter and its under-indented block).

- [ ] Indent the block to the body's first statement, with a test in
      `plaster_test.py`

### `brockit lift`: exit with failure whenever something needs attention

`brockit lift --continue` prints "❌ Plaster check. Please investigate it." and
nothing else, and still exits 0, as it does after the "❌ GNRT rerun" check. A
caller (CI, or `autolift.py lift`) can't tell a finished lift from one that
needs attention without parsing the output. In cr158 `plaster apply --all`
afterwards changed nothing, so the cause stayed unknown.

- [ ] Exit non-zero when any "action needed" section, ❌ check or error is
      reported, so that a zero exit means the lift is done
- [ ] Show the failing plasters and the reason, from `plaster check`'s output

### `git cr follow-renames`: cover more of the move, touch less

- [ ] Update Java imports and `junit` tests for moved classes. In cr158,
      `RecentlyClosedEntriesManager`'s move needed two imports fixed by hand,
      found only by the Android build
- [ ] Run the formatter only on the files it changed. It reflowed
      `tools/cr/M157_codehealth.md`, an unrelated file, on every run

### `pnpm run update_patches <path>`: delete patches that became empty

When a conflict resolution leaves the upstream file identical to upstream,
`update_patches` leaves the old patch in place (cr158:
`net-http-transport_security_state.cc.patch`), and it has to be `git rm`ed by
hand.

- [ ] Delete the patch when the file has no diff

### `git cr commit --culprit`: write full hashes

The "Chromium changes:" block keeps a hash as given, so some commits cite
13-character hashes.

- [ ] Resolve the culprit to its full hash before writing the block
- [ ] Reject a culprit outside the branch's Chromium range (what
      `autolift.py verify-culprits` checks after the fact)

### `pnpm run build`: refuse a second build in the same out dir

Two overlapping builds in `out/Component` don't fail fast. The second one ends
with an `ENOENT` crash in `actionGuard.markFinished` on `build.guard`, after
both have run.

- [ ] Check the guard at start, and refuse while another build holds it

### `tools/recipes/runtest.py`: fail when a filter matches no tests

A `--gtest_filter` that matches nothing reports success (cr158:
`SpeedreaderBrowserTest.*`, where the fixture is `SpeedReaderBrowserTest`).

- [ ] Fail, or at least warn, when zero tests ran

### `pnpm run build`: summarise failures

Every build round starts by digging the failed steps and distinct errors out of
the log; `autolift.py build-summary` does it after the fact.

- [ ] Print that summary at the end of a failed build

### New tool: resolve Brave's Java name literals against two tags

No tool exists, so every lift resolves by hand the names Brave hardcodes as
strings: `BraveReflectionUtil.invokeMethod`/`getField` literals (37 call sites),
`BytecodeTest` `methodExists`/`fieldExists`/`constructorsMatch` strings (437
assertions) and bytecode adapter targets (~150 adapters). None fails loudly —
`BraveReflectionUtil` returns null in release builds
(`BraveReflectionUtil.java:57`), `BytecodeTest` is an on-device javatest, and a
stale adapter target never matches its map entry
(`BraveClassVisitor.java:178,234,255`) — yet the sweep has found a silent
regression in each of the last three lifts (M157, 8093.0: `isTabModelRestored`
renamed, which turned tab auto-grouping off in release; M158, 8104.1:
`setupContentSettingsPreference` gained a parameter, emptying two per-site rows;
8107.2: `UndoBarController.showUndoBar`).

Design and the rest of the evidence are in
[#60337](https://github.com/brave/brave-browser/issues/60337); the lift-side
finding is under "Script the Android names-in-strings check" above.

- [ ] Add the script, resolving each literal from source at both tags and
      separating "broke between tags" from pre-existing debt
