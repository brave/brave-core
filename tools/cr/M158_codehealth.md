# M158 code health follow-ups

Tasks found by reviewing upstream changes in `157.0.8103.3..158.0.8104.1` (362
commits) for regressions, deprecations and code-health migrations that apply to
Brave. Tick items off as they land.

## 1. Regressions

### Ads and Rewards tab helpers get an invalid tab ID and switch themselves off

Upstream moved `CreateSessionServiceTabHelper()` out of the start of
`TabHelpers::AttachTabHelpers()`, which "other helpers may rely on"
([420714afe4097](https://chromium.googlesource.com/chromium/src/+/420714afe4097abadd140d8f7159954a829cd78e)).
It now runs after `AttachTabHelpers()`: on desktop in the `TabModel`
constructor, and on Android in `TabFeatures`, which
`TabAndroid::InitWebContents` creates later. Brave's helpers are created from
`BRAVE_TAB_HELPERS` inside `AttachTabHelpers()`, so
`SessionTabHelper::IdForTab()` is invalid in their constructors and both return
early. Ads never connects to the ads service and Rewards never observes the
rewards service, and nothing reports it. They are the only Brave constructors
that read the ID (`git grep -n 'IdForTab(' -- '*.cc'`).

- [ ] `browser/brave_ads/tabs/ads_tab_helper.cc:75`: read the ID lazily, or
      create the helper from `BraveTabFeatures`
      (`browser/ui/tabs/brave_tab_features.cc`) after upstream's `Init`
- [ ] `browser/brave_rewards/rewards_tab_helper.cc:28`: same
- [ ] Android: same, in `browser/android/brave_tab_features.cc`
- [ ] Add a browser test that both helpers see a valid tab ID

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

- [ ] Give the bridge a channel to the popup that doesn't need a
      `chrome-untrusted://` → web opener (e.g. the browser opens the popup and
      relays messages). Don't reopen what upstream closed
- [ ] `components/brave_wallet_ui/trezor/trezor_patches.ts`: drop the
      `window.open` proxy once replaced
- [ ] `browser/ui/webui/brave_wallet/trezor/trezor_ui_browsertest.cc:43`:
      `CheckOpenerInPopup` asserts `!!window.opener` and should now fail; update
      it to the new design

### iOS: deleting history crashes without Google history sync

`WebHistoryServiceFactory` now always creates the service, and every request
method `CHECK(IsEnabled())`. Upstream's drivers return the service only when
`IsEnabled()`
([0c613e80df545](https://chromium.googlesource.com/chromium/src/+/0c613e80df545d2451bc8d709d64636339bb1b68)).
Brave's `HistoryDriverIOS` is a fork of `IOSBrowsingHistoryDriver` and returns
the getter's result unconditionally, so `BrowsingHistoryService::RemoveVisits()`
(called at `ios/browser/api/history/brave_history_api.mm:303`) reaches
`WebHistoryService::ExpireHistory` and crashes.

- [ ] `ios/browser/api/history/history_driver_ios.mm:65`
      `GetWebHistoryService()`: return the service only when `IsEnabled()`, as
      upstream's `ios_browsing_history_driver.mm` now does

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

- [ ] `rewrite/services/network/cors/cors_url_loader.cc.yaml`: also pass
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

## 3. Code-health migrations

### `cbor::Writer::Write` without a temporary `cbor::Value`

Upstream is passing maps and arrays to `cbor::Writer::Write` directly through
`ValueView` (e.g.
[970f30a9b8ff7](https://chromium.googlesource.com/chromium/src/+/970f30a9b8ff76dc186b58a2ca0510643c1c0bda):
`cbor::Writer::Write(map)`). `Writer::Write(cbor::Value(` fell from 69 to 28
(`git grep -E 'Writer::Write\(cbor::Value\(' <tag> -- '*.cc' '*.mm' | wc -l`).

- [ ] `components/brave_wallet/browser/cardano/cardano_cip30_serializer.cc:35,56,83,105`
- [ ] `components/brave_wallet/browser/fil_transaction.cc:271`

### `ui_public_dependencies` → direct deps

Upstream finished removing `browser_public_dependencies` and its `browser/ui`
counterpart, so targets list their direct deps
([1d4b690ab57b](https://chromium.googlesource.com/chromium/src/+/1d4b690ab57b511ef64f9bb681aef7127036573a)).
The rebase routed Brave's UI deps through a new group with the same role
("[cr158] Route the Brave UI public deps through `browser`").

- [ ] `browser/ui/BUILD.gn:39` `ui_public_dependencies` and
      `browser/sources.gni:788` `brave_chrome_browser_public_deps`: move the
      entries to the targets that use them, and verify with `gn check`

## 4. Obsolete Brave code

### Test filters for deleted tests

[064bca2b1e8bc](https://chromium.googlesource.com/chromium/src/+/064bca2b1e8bc9dfdd45d6f6175b67df938e52b7)
deleted `request_storage_access_for_browsertest.cc`, which held the
`RequestStorageAccessFor*` and `TopLevelStorageExemptionReasonMetricTest`
suites.
[22692370687c0](https://chromium.googlesource.com/chromium/src/+/22692370687c0b3d8d76a9cde2c8f7fd309dc9aa)
deleted `PasswordChangePageStabilityWaiterTest`.

- [ ] `test/filters/browser_tests.filter:908` and `:1610-1614`
      (`RequestStorageAccessFor*`)
- [ ] `test/filters/browser_tests.filter:2026-2028`
      (`TopLevelStorageExemptionReasonMetricTest`, with its comment)
- [ ] `test/filters/generated/browser_tests-win.filter:4818` and
      `test/filters/generated/unit_tests-win-asan.filter:42`: rerun
      `tools/chromium_tests_analysis/update-upstream-flake-filters.py`

### Plaster docs use the deleted target as their example

- [ ] `docs/best-practices/plaster.md:153`: replace the
      `browser_public_dependencies` example, which no longer exists upstream

## 5. Optional

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
