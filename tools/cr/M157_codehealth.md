# M157 code health follow-ups

Tasks found by reviewing upstream changes in `156.0.8078.4..157.0.8094.1` (8589
commits) for regressions, deprecations and code-health migrations that apply to
Brave. Tick items off as they land.

Cross-range migrations live here, as do pre-existing problems found during the
review, marked as such. Items specific to cr156 are in `M156_codehealth.md`.

## 1. Regressions

### Brave settings pages lose the top-level `prefs`

Upstream moved `<settings-prefs>` into `privacy_page_index.html` (so it exists
only once the privacy route has rendered), and removed the `prefs` property from
`settings-ui` and `settings-main`, along with the `refresh-pref` event
([246cb3110a161](https://chromium.googlesource.com/chromium/src/+/246cb3110a161d2d51df4f17c4cf61cf816bf744)).
Every `prefs="{{prefs}}"` that `br/settings_main.ts` injects now binds to
nothing, so Brave's `PrefsMixin` pages can't read or write their prefs. This
fails silently. The privacy and autofill items below fixed only those two pages.
`PrefsMixin` writes through the `settings-prefs` element, so re-adding one in
`settings-main` needs care: `SettingsPrefsElement.disconnectedCallback()` calls
the global `CrSettingsPrefs.resetForTesting()`, and leaving the privacy page
would reset it for Brave's instance too. Migrating to `pref-key`/`PrefService`,
upstream's direction, avoids both.

- [x] `browser/resources/settings/br/settings_main.ts:68`, `:84`, `:101`,
      `:117`, `:136`, `:155`, `:172`, `:187`, `:204`: nine injected page indexes
      pass `prefs="{{prefs}}"`
- [x] Migrate Brave's consumers to `pref-key`/`PrefServiceObserverMixin`, or
      restore a `prefs` source: 27 files use `PrefsMixin` and about 31 bind
      `pref="{{prefs.…}}"`
      (`git grep -l PrefsMixin -- browser/resources/settings`,
      `git grep -nE 'pref="\{\{prefs\.' -- browser/resources`), e.g.
      `brave_wallet_page/`, `brave_content_page/`,
      `default_brave_shields_page/`, `ad_block_only_mode_page/`,
      `brave_tor_page/`, `brave_default_extensions_page/`,
      `brave_leo_assistant_page/`, `brave_search_engines_page/`,
      `brave_system_page/brave_vpn_page.ts` and
      `brave_web3_domains_page/brave_web3_domains_page.ts:55`
- [x] `chromium_src/chrome/browser/resources/settings/search_page/search_page.ts:46`
      and `:115`: `getPrefsElement()` looks for `settings-prefs` in
      `settings-ui`'s shadow root and gets null, so `bravePrefs_` stays unset.
      `search_page.html.ts.lit_mangler.ts` gates `<settings-brave-search-page>`
      on it, so the private search card never renders
- [x] Add a WebUI or browser test that flips one toggle on a Brave page and
      checks the pref

### Brave's privacy page override no longer applies

`settings-privacy-page` is now a `CrLitElement`
([61ef0da501a86](https://chromium.googlesource.com/chromium/src/+/61ef0da501a8645e26c94cd369690df2970da834)),
so the "Privacy page toggles lost their pref binding" item below, which says
it's still Polymer, is out of date. `polymer_overriding.ts` only hooks Polymer
templates, so `browser/resources/settings/br/privacy_page.ts` does nothing. This
fails silently:

- `<settings-brave-personalization-options>` (history embeddings, GCM, de-AMP,
  debounce, reduce-language and PSST toggles) is no longer on the privacy page.
- `#thirdPartyCookiesLinkRow` is no longer hidden.
- `#privacyGuideLinkRow` is no longer hidden (it is behind
  `isPrivacyGuideAvailable`).

- [x] Port `br/privacy_page.ts` to
      `chromium_src/chrome/browser/resources/settings/privacy_page/privacy_page.html.ts.lit_mangler.ts`,
      as for `security_page` and `add_site_dialog`: insert the personalization
      options after `#siteSettingsLinkRow` (it binds with `pref-key`, no
      `prefs`), and hide both link rows. `#privacyGuideLinkRow` is in a nested
      template, so it needs its own `mangle` with a template predicate
- [x] Delete `br/privacy_page.ts` and its import in `br/index.ts`

### Settings menu, layout and autofill overrides no longer apply

`settings-ui`
([efffbf3d3080d](https://chromium.googlesource.com/chromium/src/+/efffbf3d3080d634c029b5f938b4c914d6905798)),
`settings-menu`
([296e1705e95bc](https://chromium.googlesource.com/chromium/src/+/296e1705e95bc1a4698374ca0a830969c73095c3))
and `settings-autofill-page`/`settings-autofill-page-index`
([3c7338f81eaee](https://chromium.googlesource.com/chromium/src/+/3c7338f81eaeeea5ffa52a2bbebd3ae4f0fca7df))
are now `CrLitElement`s, so their Polymer overrides do nothing apart from the
`polymer_overriding.ts` error. TypeScript still compiles; 297e21e9e12 only
dropped the `shadowRoot` assertions. The autofill templates are now
`autofill_page.html.ts` and `autofill_page_index.html.ts`
([51c50f378832e](https://chromium.googlesource.com/chromium/src/+/51c50f378832eab4594ada4fe8570c17868f1d17)),
which is what the lit_manglers must target. This undoes the two autofill items
below.

- [x] `browser/resources/settings/br/settings_menu.ts:229`: the menu loses Get
      Started, Origin, Content, Shields, Web3, Leo, Sync and Extensions, the
      Appearance/Privacy/Search/Autofill reordering, the Performance and
      `#extensionsLink` removals, and the version under About. The style
      override at `:55` is dead too. Port to `settings_menu.html.lit_mangler.ts`
      and `injectStyle`; every selector still exists
- [x] `browser/resources/settings/br/settings_ui.ts:49` (Leo layout styles),
      `:176` (behaviors, including `setGlobalScrollTarget(this.$.main)`; Lit
      `settings-ui` now sets `#container` in `connectedCallback`) and `:183`
      (`<leo-alertcenter>`). This supersedes the `showing-section` item under
      "Pre-existing: stale overrides"
- [x] `browser/resources/settings/br/autofill_page.ts:40` (style), `:86`
      (template), `:194` (`-index` template), `:214` and `:234` (prototype
      modifications): the Email Aliases card and subpage, the
      `brave.autofill_private_windows` toggle, and the hidden title, account,
      identity/travel cards, related services and Autofill AI section.
      `on-data-category-click` becomes a Lit listener; `getAssociatedControlFor`
      and `currentRouteChanged` still exist for prototype wraps. The
      `CategoryReferenceCardElement` `injectStyle` at `:28` is fine
- [x] Rebuild, then run
      `pnpm run test-unit -t "mangled files should have up to date snapshots" -u`
      and check the round-trip in the new `settings_menu`, `settings_ui`,
      `autofill_page` and `autofill_page_index` snapshots. They read the built
      output, so they couldn't be generated without a build

### Android: Settings crashes with "Autofill and passwords" enabled

Upstream enabled `kYourSavedInfoSettingsPageAndroid`
([7e86550350b95](https://chromium.googlesource.com/chromium/src/+/7e86550350b95169649d22b64dd5c1be702e2a2f)).
`MainSettings.updateAutofillAndPasswords()` then removes `autofill_section`,
`passwords`, `autofill_payment_methods`, `autofill_addresses` and
`autofill_options` through its own private `removePreferenceIfPresent`, so they
never reach Brave's `mRemovedPreferences` fallback in `findPreference`.
`BraveMainPreferencesBase.rearrangePreferenceOrders()` then calls
`setPreferenceOrder()` on them, whose `assumeNonNull` is a no-op at runtime, so
opening Settings throws a `NullPointerException`. The new
`autofill_and_passwords` row keeps its XML order inside Brave's features
section. Android-only.

- [ ] Pin `kYourSavedInfoSettingsPageAndroid` disabled (a
      `chrome_feature_list.cc` plaster, `base/compile_overridden_features.inc`
      and `app/feature_defaults_unittest.cc`), or adopt it:
- [ ] If adopting: order `MainSettings.PREF_AUTOFILL_AND_PASSWORDS` in
      `android/java/org/chromium/chrome/browser/settings/BraveMainPreferencesBase.java:417-421`
      when the flag is on, and review what `AutofillAndPasswordsFragment` shows
      (Google Wallet, Autofill AI, where passwords lead)

### Android: `countrySupplier` removed from toolbar and bottom bar constructors

Upstream removed bottom bar geofencing, and with it the
`OneshotSupplier<String> countrySupplier` parameter of `ToolbarManager`,
`BottomBarContainerCoordinator` and `BottomBarCoordinator`
([501a3afe1d6ea](https://chromium.googlesource.com/chromium/src/+/501a3afe1d6ea73c746a44c7db9b3321e02a6269)).
The regenerated patches call Brave's subclasses without it, but the subclasses
still declare and forward it. Android-only, so the Linux build doesn't catch it.
Glic stays off through Brave's `GlicEnabling` plaster.

- [ ] `android/java/org/chromium/chrome/browser/toolbar/BraveToolbarManager.java:277`,
      `:341`
- [ ] `browser/ui/android/bottombar/java/src/org/chromium/chrome/browser/ui/bottombar/BraveBottomBarCoordinator.java:59`,
      `:71`, and the `OneshotSupplier` import
- [ ] `android/javatests/org/chromium/chrome/browser/BytecodeTest.java:1574`:
      drop the `OneshotSupplier.class` before `GlicButtonDelegate.class`

### Android: two "Show bottom toolbar" switches in Appearance

Upstream added a `bottom_bar_switch` to Appearance settings, shown when
`BottomBarConfigUtils.shouldShowSettingsToggle()` is true
([1d84253f53296](https://chromium.googlesource.com/chromium/src/+/1d84253f53296fce8a42a89049a8ec94ed715b3c)).
That is when `AndroidBottomBar` is on, which is exactly when
`AppearancePreferences` keeps Brave's `BRAVE_ENABLE_BOTTOM_BAR` switch. Both
then show, and both must be on, since Brave's plaster ANDs its pref into
`isBottomBarUserEnabled()`. Brave's `SEARCH_INDEX_DATA_PROVIDER` replaces
upstream's and doesn't drop the new entry, so settings search lists it even with
the flag off.

- [ ] `android/java/org/chromium/chrome/browser/settings/AppearancePreferences.java:99-104`:
      remove `AppearanceSettingsFragment.PREF_BOTTOM_BAR_SWITCH`, or back
      Brave's switch with `BottomBarConfigUtils.setBottomBarUserEnabled()` and
      drop the plaster's AND
- [ ] Same file, `updateDynamicPreferences`: remove `PREF_BOTTOM_BAR_SWITCH`
      from the index

### Shared pinned tabs miss blocked-state changes

Upstream removed `TabChangeType::kBlockedOnly`: `TabStripModel::SetTabBlocked()`
now calls the new `TabStripModelObserver::OnTabBlockedStateChanged()` instead of
`OnTabChangedAt()`
([b5cc7c8ecc923](https://chromium.googlesource.com/chromium/src/+/b5cc7c8ecc923050e6dfcea67853cc2a4f5fe4bf)).
`SharedPinnedTabService::OnTabChangedAt` refreshes `renderer_data` and pushes it
to the other windows, so their dummy pinned tabs no longer show the
blocked-dialog attention icon.

- [x] `browser/ui/tabs/shared_pinned_tab_service.h:84`, `.cc:423`: override
      `OnTabBlockedStateChanged()` with the same propagation
- [x] `browser/ui/brave_browser_command_controller.cc:174`:
      `BrowserCommandController` no longer overrides `OnTabChangedAt`, so the
      super call reaches the empty `TabStripModelObserver` default; drop it

### Autofill category cards show chips again

`category-reference-card` is now Lit
([5b22c54fe23de](https://chromium.googlesource.com/chromium/src/+/5b22c54fe23de449fef61261bdb837269ca90268)),
so the `<style>` that `br/autofill_page.ts:181` appends to its Polymer template,
hiding `hr` and `.chips-container`, does nothing. The chips and separator come
back on every category card, including Brave's `#emailAliasesCard`. The injected
card itself should still work (`card-title` maps to `cardTitle`, and
`on-data-category-click` bubbles).

- [x] `browser/resources/settings/br/autofill_page.ts:181`: drop the
      `category-reference-card` entry, and hide the two parts with `injectStyle`
      from `lit_overriding.js`, as `appearance_page.ts` does

The autofill page itself went Lit in 157.0.8094.1; see "Settings menu, layout
and autofill overrides no longer apply".

### Android: tab auto-grouping and `BytecodeTest` use renamed `TabModel` methods

Upstream renamed `TabCollectionTabModelImpl.isTabModelRestored()` to
`isTabStateInitialized()`
([02950b80d63a5](https://chromium.googlesource.com/chromium/src/+/02950b80d63a5b69c5417c60aa6490ff2d7be6ff))
and `TabModel.tabGroupExists()` to `containsTabGroup()`
([ce6ee0bb9b2d0](https://chromium.googlesource.com/chromium/src/+/ce6ee0bb9b2d00f1157040f2c857430a21f81653)).
`BraveTabCollectionTabModelImplBase` calls the first by name through
`BraveReflectionUtil`, which asserts in debug builds and returns null in release
ones, so `shouldGroupWithParent` never fires and link-clicked tabs stop being
grouped with their parent. This fails silently, and the Linux build doesn't see
it. The junit tests still call the old names too.

- [x] `android/java/org/chromium/chrome/browser/tabmodel/BraveTabCollectionTabModelImplBase.java:57`:
      reflect `"isTabStateInitialized"`, or call it directly if it's visible
      (c0fbf3b62d4, called directly)
- [x] `android/javatests/org/chromium/chrome/browser/BytecodeTest.java:1387`:
      update the `methodExists` name (c0fbf3b62d4)
- [x] `android/junit/src/org/chromium/chrome/browser/toolbar/top/BraveTabSwitcherActionMenuCoordinatorTest.java:79`
      (`isTabModelRestored`),
      `android/junit/src/org/chromium/chrome/browser/tasks/tab_management/BraveTabGridContextMenuCoordinatorTest.java:111`
      and `:112`, and
      `android/junit/src/org/chromium/chrome/browser/tasks/tab_management/pinned_tabs/BravePinnedTabStripItemContextMenuCoordinatorTest.java:98`
      (`tabGroupExists`, 3f4a2e9c286; `isTabModelRestored`, c0fbf3b62d4)

### Android: YouTube picture-in-picture no longer checks the Android version

`PictureInPicture.isEnabled()` used to return false below Android R, to avoid
the framework crash b/143784148. The unified PiP checks dropped that, and its
Javadoc now says callers needing a minimum version must check it themselves; the
web PiP gate does
(`ActivityTabWebContentsDelegateAndroid#isPictureInPictureEnabled`)
([6935629f66706](https://chromium.googlesource.com/chromium/src/+/6935629f66706d8c54723d4bb9792d642f5893bc),
relanding 3be277cafff63 after 55d6b8b7eaaa4). Brave's own PiP entry points rely
on `isEnabled()` and have no SDK check, and the Chromium minimum SDK is 29, so
Android 10 devices now get the YouTube PiP icon and entry. Brave's comment at
`BraveYouTubeScriptInjectorNativeHelper.java:52` ("Covers SDK < R") is now
wrong.

- [x] Add `Build.VERSION.SDK_INT >= Build.VERSION_CODES.R` (or one shared
      helper) at
      `android/java/org/chromium/chrome/browser/youtube_script_injector/BraveYouTubeScriptInjectorNativeHelper.java:55`,
      `android/java/org/chromium/chrome/browser/toolbar/top/BraveToolbarLayoutImpl.java:511`,
      `:639` and `:1130`, and
      `android/javatests/org/chromium/chrome/browser/toolbar/BraveYouTubePipIconTest.java:64`
      (901e4cd3453, via
      `BraveYouTubeScriptInjectorNativeHelper.isPictureInPictureEnabled`)
- [x] Check `BraveYouTubePictureInPictureController.java:673`, which goes
      through the native availability check: it only re-enters PiP after an
      earlier entry, which the helper already gated

### Brave chrome-untrusted:// pages can't load images or media from other hosts

`WebUIURLLoaderFactory` now locks a `chrome-untrusted://` frame's subresource
factory to the frame's origin. It rejects loads from another
`chrome-untrusted://` host with `ERR_FAILED`, including no-cors `<img>` and
`<video>`, unless the target data source's
`GetAccessControlAllowOriginForOrigin()` returns `*` or the initiator
([738f50bbfc2b9](https://chromium.googlesource.com/chromium/src/+/738f50bbfc2b9)).
Upstream opened up only its own sources (shared resources, theme, favicon2 for
data-sharing). The rebase plumbed the origin lock through, but no Brave data
source overrides the method. `PlaylistDataSource` subclasses `FaviconSource`, so
it inherits favicon2's data-sharing-only rule. This fails silently. Follow
`ThemeSource::GetAccessControlAllowOriginForOrigin()` and
`FaviconSource::GetAccessControlAllowOriginForOrigin()`.

- [x] `browser/playlist/playlist_data_source.h`: allow playlist and
      playlist-player (media, thumbnails, favicons;
      `components/playlist/content/browser/resources/utils/urlFixer.ts`)
- [x] `browser/ui/webui/untrusted_sanitized_image_source.h`
      (chrome-untrusted://image): allow nft-display, market-display and
      leo-ai-conversation-entries
- [x] `browser/ui/webui/brave_sanitized_image_source.h`
      (chrome-untrusted://brave-image, when `serve_untrusted_`): allow news
- [x] chrome-untrusted://favicon2 for leo-ai-conversation-entries
      (`browser/ui/webui/ai_chat/ai_chat_untrusted_conversation_ui.cc:508`):
      plaster in `FaviconSource`, since upstream's Data Sharing page registers
      the same source and would replace a Brave subclass
- [x] Add a browser test per page that loads one cross-origin image or media
      URL: `PlaylistBrowserTest.UntrustedPagesLoadPlaylistData`,
      `UntrustedSanitizedImageSourceBrowserTest.WalletPagesLoadImages`,
      `AIChatConversationEntriesBrowserTest.LoadsUntrustedImages` and
      `BraveNewsUIBrowserTest.LoadsBraveImage`

### Privacy page toggles lost their pref binding

Upstream replaced `PrefsMixin` with `PrefServiceObserverMixin` on
`settings-privacy-page`, and `privacy_page_index.html` no longer passes `prefs`
to it
([d82f1283a958e](https://chromium.googlesource.com/chromium/src/+/d82f1283a958e)).
The element was still Polymer then, so `br/privacy_page.ts:31` injected
`<settings-brave-personalization-options prefs="{{prefs}}">`, but `prefs` was
undefined (it has since become Lit, see above). The toggles in
`brave_personalization_options.html` render, but aren't tied to their prefs.
This fails silently. The rebase fixed only the cookies page (97a2e81595f).

- [x] `browser/resources/settings/brave_privacy_page/brave_personalization_options.html`:
      bind with `pref-key` at `:17` (`brave.history_embeddings_enabled`), `:59`
      (`brave.gcm.channel_status`), `:70` (`brave.de_amp.enabled`), `:78`
      (`brave.debounce.enabled`), `:86` (`brave.reduce_language`) and `:103`
      (`brave.psst.settings.enable_psst`)
- [x] The restart prompts at `:22` and `:62` read `prefs.….value`: add
      `PrefServiceObserverMixin` and `mirrorPref()` to
      `brave_personalization_options.ts`, as upstream `privacy_page.ts` does for
      `profile.cookie_controls_mode`
- [x] Drop `prefs="{{prefs}}"` from `br/privacy_page.ts:31`, and from
      `<settings-personalization-options>` in
      `patches/chrome-browser-resources-settings-privacy_page-privacy_page.html.patch`
      (Lit, no `prefs` property)

### Android toolbar shows a Google sign-in button

Upstream enabled `kSigninLevelUpButton` and `kProfileDiscOnAllPages`, in C++ and
in the `SigninFeatureMap` cached defaults
([3e6167c2c73e1](https://chromium.googlesource.com/chromium/src/+/3e6167c2c73e1)).
`AdaptiveToolbarUiCoordinator` then stops creating `IdentityDiscController`,
whose `calculateButtonData()` Brave no-ops
(`android/java/org/chromium/chrome/browser/identity_disc/BraveIdentityDiscController.java:48`).
Instead `TopToolbarCoordinator` inflates a `SigninButtonCoordinator` from
`signin_button_stub`, which Brave's toolbar layouts keep. Its visibility doesn't
check whether sign-in is allowed: it shows a placeholder avatar on the NTP, and
on every page on tablets. Tapping it starts Google sign-in.

- [x] `rewrite/components/signin/public/base/signin_switches.cc.yaml`: ship both
      flags disabled, with `base/compile_overridden_features.inc` and
      `app/feature_defaults_unittest.cc`
- [x] `components/cached_flags/android/java/src/org/chromium/components/cached_flags/BraveCachedFlag.java:30`:
      `SigninLevelUpButton` and `ProfileDiscOnAllPages` → false

### Autofill page toggles lost their pref binding

Upstream removed `PrefsMixin` and the `prefs` property from
`settings-autofill-page`
([8704780af8d79](https://chromium.googlesource.com/chromium/src/+/8704780af8d79)).
`br/autofill_page.ts` still injects a toggle bound with
`pref="{{prefs.brave.autofill_private_windows}}"`, which now resolves against a
property that no longer exists, so the toggle is not tied to the pref. This
fails silently. Other Brave toggles in Polymer templates use `pref-key` (e.g.
`brave_personalization_options.html`). The same commit dropped `prefs` from
`settings-autofill-page-index`, and `settings-main` no longer passes it down, so
the Email Aliases page Brave injects there lost its prefs too.

- [x] `browser/resources/settings/br/autofill_page.ts:177`: replace `pref` with
      `pref-key="brave.autofill_private_windows"`
- [x] Email Aliases toggle: bind
      `pref-key="brave.email_aliases.new_alias_autofill_suggestion_enabled"` in
      `browser/resources/settings/email_aliases_page/email_aliases_page.html:8`,
      drop `PrefsMixin` from `email_aliases_page.ts`, and drop
      `prefs="{{prefs}}"` from the injection in `br/autofill_page.ts:206`

Both fixes live in Polymer overrides that stopped applying in 157.0.8094.1; see
"Settings menu, layout and autofill overrides no longer apply".

### Brave Origin page extended the removed Polymer `RelaunchMixin`

Upstream removed the Polymer `RelaunchMixin`
([f8a31c97957df](https://chromium.googlesource.com/chromium/src/+/f8a31c97957df)).
The page failed `tsc` on the missing module.

- [x] Migrate
      `browser/resources/settings/brave_origin_page/brave_origin_page.ts` to Lit
      with `RelaunchMixinLit`

### Security page overrides no longer apply

Upstream migrated `settings-security-page` to Lit
([7b7de39620ae0](https://chromium.googlesource.com/chromium/src/+/7b7de39620ae0),
[a47b9d1ed1570](https://chromium.googlesource.com/chromium/src/+/a47b9d1ed1570)).
`browser/resources/settings/br/security_page.ts` still uses
`RegisterPolymerTemplateModifications`, which never runs for a Lit element, so
the hidden settings are visible again. This fails silently.

- [x] Port `br/security_page.ts` to
      `chromium_src/chrome/browser/resources/settings/privacy_page/security/security_page.html.lit_mangler.ts`
      and remove the Polymer override:
  - [x] Hide `safeBrowsingReportingToggle`.
  - [x] Hide `safeBrowsingEnhanced`.
  - [x] Set `no-collapse` on `safeBrowsingStandard`.
  - [x] Hide `passwordsLeakToggle`.
  - [x] Hide `httpsOnlyModeToggle` when `isHttpsByDefaultEnabled`.
  - [x] Hide `advancedProtectionProgramLink`.

### Contact info page shows an empty email verification card

Upstream migrated `settings-contact-info-page` to Lit
([cb62481537773](https://chromium.googlesource.com/chromium/src/+/cb62481537773)).
`browser/resources/settings/br/contact_info_page.ts` hid the card around the
email verification block, and no longer runs. With `kEmailVerificationProtocol`
off in Brave, the card renders with only its heading.

- [x] Port `br/contact_info_page.ts` to a lit_mangler for
      `autofill_page/contact_info/contact_info_page.html.ts` that hides the
      `.card` containing `#emailSharedMenu` (declared in `$`, so hide it rather
      than remove it), and remove the Polymer override

### Upstream Ctrl+Tab MRU toggle duplicates Brave's MRU cycling

Upstream enabled `kCtrlTabMru` on desktop
([d97db62955747](https://chromium.googlesource.com/chromium/src/+/d97db62955747)).
Appearance settings now show its toggle (`browser.ctrl_tab_mru`). When it's on,
`IDC_CYCLE_TO_NEXT_TAB`/`PREV_TAB` go to the cross-window `CycleToMruTab`,
bypassing `BraveTabStripModel::SelectNextTab`, which implements Brave's own
`brave.mru_cycling_enabled`. Users now get two settings with different
behaviour.

- [ ] `rewrite/chrome/browser/ui/ui_features.cc.yaml`: ship `kCtrlTabMru`
      disabled, with `base/compile_overridden_features.inc` and
      `app/feature_defaults_unittest.cc`, which also hides the toggle
- [ ] Longer term: decide whether to move `brave.mru_cycling_enabled` onto
      upstream's pref and drop Brave's MRU code
      (`browser/ui/tabs/brave_tab_strip_model.cc:80`)

### File System Access `AsBlob()` security check skipped

Upstream now re-checks sensitive paths in
`FileSystemAccessFileHandleImpl::AsBlob()` (a handle can be swapped for a
symlink to a sensitive path), but only when
`kFileSystemAccessDirectoryIterationBlocklistCheck` is on
([a93b504228a53](https://chromium.googlesource.com/chromium/src/+/a93b504228a53)).
Brave has shipped that flag disabled since 2023, so it opts out of the fix.
Local FSA is off unless `kFileSystemAccessAPI` is enabled.

- [x] Security team: re-evaluate the override in
      `rewrite/content/browser/file_system_access/features.cc.yaml:7`, and
      likely drop it (with its `base/compile_overridden_features.inc:70` entry):
      dropped in fed01c64f8a

### Android: `setKeyboardVisibility(boolean, boolean)` removed

Upstream dropped the `delayHide` parameter
([64decd59a5eb1](https://chromium.googlesource.com/chromium/src/+/64decd59a5eb1)).
Android-only, so the Linux build doesn't catch it.

- [x] `android/java/org/chromium/chrome/browser/omnibox/BraveLocationBarMediator.java:275`:
      `setKeyboardVisibility(true)` (cbb9a383d4b)

### iOS: `HostContentSettingsMapFactory::GetForProfile()` returns `scoped_refptr`

`ios::HostContentSettingsMapFactory::GetForProfile()` now returns
`scoped_refptr<HostContentSettingsMap>`
([2b7b28cb5acd7](https://chromium.googlesource.com/chromium/src/+/2b7b28cb5acd7)),
which doesn't convert to a raw pointer. iOS-only, so the Linux build doesn't
catch it. Hold a `scoped_refptr` local, or call `.get()` where a raw pointer is
stored.

- [x] `ios/browser/brave_wallet/ethereum_provider_tab_helper.mm:133`
      (e7f559a2228)
- [x] `ios/browser/brave_shields/brave_shields_settings_service_factory.mm:44`
      (e7f559a2228)
- [x] `ios/browser/api/brave_wallet/brave_wallet_api.mm:93`, `:121`
      (e7f559a2228)
- [x] `ios/browser/content_settings/content_settings_browsing_data_utils.mm:21`
      (e7f559a2228)
- [x] `ios/app/brave_profile_controller.mm:325` (e7f559a2228)

### iOS: new multiwindow provider missing from `brave_providers`

`scene_delegate.mm` now calls `ios::provider::IsWindowSceneActivationAllowed()`,
and upstream added `chromium_multiwindow` to its providers list
([9926f1ac83b16](https://chromium.googlesource.com/chromium/src/+/9926f1ac83b16)).
Brave's copy of that list lacks it, so the app likely fails to link. Brave
needed the same fix in cr154 and cr155.

- [x] `ios/browser/providers/BUILD.gn:40`: add
      `"//ios/chrome/browser/providers/multiwindow:chromium_multiwindow"`
- [x] Also add
      `//ios/chrome/browser/providers/switcher_info:chromium_switcher_info`
      ([339ec6f1b3c10](https://chromium.googlesource.com/chromium/src/+/339ec6f1b3c1077fc49c12165eda35800fe421f1)).
      Nothing in `ios/chrome` calls it yet, so it only fails to link once a
      consumer lands

### Android: `BraveAutocompleteMediator` constructor is missing a parameter

`AutocompleteMediator` and
`AutocompleteCoordinator.createAutocompleteMediator()` gained a trailing
`NonNullObservableSupplier<Boolean> urlTextWrappingSupplier`
([6e3b15b782f6d](https://chromium.googlesource.com/chromium/src/+/6e3b15b782f6dbeeecf2d3ab23c45b0414d7f371)).
Brave's subclass declares the old 19 parameters and calls `super(...)` with
them, so it doesn't compile, and the bytecode constructor redirect to it needs
matching descriptors. Android-only, so the Linux build doesn't catch it.

- [x] `android/java/org/chromium/chrome/browser/omnibox/suggestions/BraveAutocompleteMediator.java:63`
      and the `super(...)` call at `:84`: add the parameter (and its
      `org.chromium.base.supplier.NonNullObservableSupplier` import)
      (fe014ed2302)
- [x] Check the `AutocompleteMediator` redirect in
      `build/android/bytecode/java/org/brave/bytecode/BraveAutocompleteMediatorClassAdapter.java`
      and `BytecodeTest.java` for constructor descriptors: the redirect names no
      descriptor, and `BytecodeTest` was updated (fe014ed2302)

### Android: `BraveBottomBarCoordinator` no longer compiles

Upstream removed `BottomBarConfigUtils.shouldIncludeHomeButtonIfEnabled()`, the
`keep_home_button_in_toolbar` param and the `shouldIncludeHomeButton` plumbing,
so the bottom bar always hosts the home button
([19d9ac5acac11](https://chromium.googlesource.com/chromium/src/+/19d9ac5acac1113ea60a6b90d842c35716826ca0)).
`BottomBarCoordinator.createActionConfigs()` takes only the view. Brave's
subclass still overrides the two-argument form and calls the removed helper.
Android-only, so the Linux build doesn't catch it. The rebase fixed the
neighbouring `disable_on_ntp` removal (68e8d68d4ea) and the user-pref check, not
this.

- [x] `browser/ui/android/bottombar/java/src/org/chromium/chrome/browser/ui/bottombar/BraveBottomBarCoordinator.java:80`:
      drop the `shouldIncludeHomeButtonIfEnabled()` guard and always register
      the observer (4b35edefb7a)
- [x] Same file, `:94`: override `createActionConfigs(BottomBarView view)` and
      call `super.createActionConfigs(view)` (4b35edefb7a)
- [x] Same file, `:130`: make the `if (shouldIncludeHomeButton)` block
      unconditional, and fix the comments at `:72` and `:128` about the 1C
      variations (4b35edefb7a)

### Android sync settings show "Search AI Mode and connected apps"

Upstream added a `search_ai_mode_connected_apps` row to the account settings,
which opens myactivity.google.com
([c7cdb801abca5](https://chromium.googlesource.com/chromium/src/+/c7cdb801abca5)).
`BraveManageSyncSettings` removes Google-only rows by key, and doesn't know this
one. Its string was also rebranded to "Brave Workspace apps, like Google Drive".

- [x] `android/java/org/chromium/chrome/browser/sync/settings/BraveManageSyncSettings.java:102`:
      `removePreferenceByKey(PREF_SEARCH_AI_MODE_CONNECTED_APPS)` (5ff87068494)

### Vertical tab strip announces stale tab positions

Upstream now updates each tab's accessible position at the end of `Layout()`
instead of on every change
([93d4870030666](https://chromium.googlesource.com/chromium/src/+/93d4870030666),
[f5af3a6317907](https://chromium.googlesource.com/chromium/src/+/f5af3a6317907)).
`BraveTabContainer::Layout()` returns early in scroll mode when its size hasn't
changed. Once the strip overflows its size stays fixed, so screen readers keep
announcing an outdated "tab X of N".

- [x] `browser/ui/views/tabs/brave_tab_container.cc:738` and `:743`: call
      `UpdateAccessibleTabIndicesIfNeeded()` on both early returns

### WebHID no longer works in container tabs

Upstream now refuses WebHID for http(s) frames whose main frame isn't in the
default StoragePartition, because device grants are per profile and would leak
across partitions
([53af426539988](https://chromium.googlesource.com/chromium/src/+/53af426539988)).
Container tabs use their own partition, so `navigator.hid` silently gets no
service there. Brave exempts containers only in
`PermissionUtil::IsPermissionBlockedInPartition`
(`chromium_src/components/permissions/permission_util.cc:211`), which this check
doesn't go through. WebUSB and Web Serial were already blocked the same way.

- [x] Decide whether device APIs stay off in containers. Allowing them means
      overriding `IsHidAllowedForFrame` in `browser/hid/brave_hid_delegate.h:23`
      (and the USB and Serial equivalents) with per-container grant scoping.
      Decided to keep them off, as for USB and Serial
- [x] Pin the chosen behaviour in `browser/containers/containers_browsertest.cc`

### Blink decision model API no longer disabled

Upstream renamed the runtime feature `AIClassifierAPI` to `AIDecisionModelAPI`
and added `AIDecisionModelAPIForWorkers`
([441b4c122840e](https://chromium.googlesource.com/chromium/src/+/441b4c122840e)).
An unknown name in `EnableFeatureFromString` only logs in debug builds, so
Brave's disablement silently does nothing. The API is still experimental only.

- [x] `renderer/brave_content_renderer_client.cc:146`: disable
      `AIDecisionModelAPI` and `AIDecisionModelAPIForWorkers`. It matters more
      now that `chrome://flags#decisions-api` can turn the API on
      ([8651d46cbd8ad](https://chromium.googlesource.com/chromium/src/+/8651d46cbd8ad6a3ae4ed6859892094250e36f09))

### Linux GTK/Qt themes give the location icon chip a background

Upstream's Linux native mixer now sets `kColorOmniboxIconBackground` and
`kColorOmniboxIconForeground` for contrast
([8b690db5c7096](https://chromium.googlesource.com/chromium/src/+/8b690db5c7096)).
It runs last in `AddChromeColorMixers()`, after Brave's
`AddBraveOmniboxColorMixer`, which makes the chip transparent
(`browser/ui/color/brave_color_mixer.cc:774`). With the GTK or Qt theme, likely
with light palettes only, the chip gets upstream's tinted background again.
Private windows aren't affected.

- [x] Re-apply Brave's chip colors after `AddNativeChromeColorMixer`, e.g. from
      `chromium_src/chrome/browser/ui/color/chrome_color_mixers.cc`
      (2ce1056f8a0, in `native_chrome_color_mixer_linux.cc`)

### Pre-existing: image metadata strip crashes if the file chooser is cancelled

Brave's `NotifyListenerAndEnd` plaster
(`rewrite/chrome/browser/file_select_helper.cc.yaml`) strips images
asynchronously and re-enters `NotifyListenerAndEnd` when done. If
`RunFileChooserEnd()` runs in between, it resets `listener_`, and the re-entry
calls `FileSelected()` on null. A tab switch could already trigger this.
Upstream now also cancels when the window shrinks below 400x300
([05fcbe537a915](https://chromium.googlesource.com/chromium/src/+/05fcbe537a915)).
`kStripImageMetadataV1` is disabled by default.

- [ ] Bail out on re-entry when `listener_` is null, and delete the stripped
      copies
      (`browser/file_select/brave_file_select_image_metadata_stripper.cc`)

### Pre-existing: Polymer overrides of elements that became Lit before cr156

These elements went Lit in earlier lifts, so their Polymer overrides do nothing.
`polymer_overriding.ts` doesn't report it.

- [x] `browser/resources/settings/br/sync_controls.ts:12`: the AI Chat sync
      toggle (`kBraveSyncAIChat`) is never injected, and the payments toggle
      isn't removed (Lit since
      [6133554291740](https://chromium.googlesource.com/chromium/src/+/6133554291740))
- [x] `browser/resources/settings/br/performance_page.ts:8`:
      `#discardRingTreatmentToggleButton` is no longer removed (Lit since
      [7e42745b4fec1](https://chromium.googlesource.com/chromium/src/+/7e42745b4fec1))
- [x] `browser/resources/settings/br/edit_dictionary_page.ts:9`: style override
      (Lit since
      [e5e2d063e35df](https://chromium.googlesource.com/chromium/src/+/e5e2d063e35df))
- [x] `browser/resources/settings/br/reset_profile_dialog.ts:10`:
      `#sendSettings` is no longer hidden and unchecked. The upload itself stays
      blocked by
      `patches/chrome-browser-profile_resetter-reset_report_uploader.cc.patch`.
      (Lit since
      [cb3ba35217a6c](https://chromium.googlesource.com/chromium/src/+/cb3ba35217a6c))
- [x] `chromium_src/chrome/browser/resources/side_panel/bookmarks/power_bookmarks_list.ts:41`:
      the refresh after moving a bookmark in custom order never runs (Lit since
      [a1ded79ae4558](https://chromium.googlesource.com/chromium/src/+/a1ded79ae4558))
- [x] `ui/webui/resources/polymer_overriding.ts`: log an error, and fail tests,
      when an element with registered modifications isn't a Polymer element

### Pre-existing: close-window warning stops after a cancelled quit

`chromium_src/chrome/browser/lifetime/browser_close_manager.cc` renames
`CancelBrowserClose` to `CancelBrowserClose_ChromiumImpl`. Upstream only calls
that private method from inside the same file, so the renamed calls skip Brave's
wrapper, and `g_browser_closing_started` stays set after the user cancels a
quit. From then on `BraveBrowser::ShouldAskForBrowserClosingBeforeHandlers()`
(`browser/ui/brave_browser.cc:188`) never warns again in that session.

- [x] Replace both `_ChromiumImpl` renames with plaster `preempt_function_impl`
      on `StartClosingBrowsers` and `CancelBrowserClose`, as
      `rewrite/chrome/browser/ui/unload_controller.cc.yaml` does

### Pre-existing: Android reader mode prompt on regular tabs

`TabImpl.isCustomTab()` now asks the tab's delegate factory instead of the
activity
([fbcb497d5ad4f](https://chromium.googlesource.com/chromium/src/+/fbcb497d5ad4f),
M153), so `BraveActivity.spoofCustomTab()` no longer affects
`ReaderModeManager.shouldUseReaderModeMessages()`.

- [ ] `android/java/org/chromium/chrome/browser/dom_distiller/BraveReaderModeManager.java:41`:
      redirect `shouldUseReaderModeMessages()` with `changeMethodOwner`, or drop
      the feature and `BraveActivity.java:3060`

### Pre-existing: iOS WebUI not cleared when leaving a WebUI page

Upstream clears `web_ui_` when a navigation fails or leaves WebUI
([b21788e67d345](https://chromium.googlesource.com/chromium/src/+/b21788e67d345)),
but Brave keeps WebUIs per frame in `web_uis_`, so they are never cleared.

- [ ] `rewrite/ios/web/web_state/web_state_impl_realized_web_state.mm.yaml`:
      make the check in `OnNavigationFinished` test `web_uis_`

### Pre-existing: `--use-system-proxy-resolver` bypasses Tor's proxy

With the switch, `SystemNetworkContextManager` gives every profile's network
context, Tor included, a `system_proxy_resolver`
(`chrome/browser/net/system_network_context_manager.cc:1031-1045`,
Win/Mac/Linux). The network service then builds a
`{Win,Mac,Linux}SystemProxyResolutionService`, which ignores the
`ProxyConfigService` where Brave injects Tor's proxy
(`chromium_src/chrome/browser/net/proxy_config_monitor.cc:21`), and skips the
circuit isolation in
`chromium_src/net/proxy_resolution/configured_proxy_resolution_service.cc:26`.
Upstream wired the same path for Linux
([c6814bd33f56a](https://chromium.googlesource.com/chromium/src/+/c6814bd33f56a),
[87c50bd142bab](https://chromium.googlesource.com/chromium/src/+/87c50bd142bab)),
and the portal-based Linux resolver (`ChromeMojoProxyResolverLinux`, when
`use_dbus`) landed in 157.0.8093.0
([36800f11c153a](https://chromium.googlesource.com/chromium/src/+/36800f11c153a),
relanded after a revert), so the bypass now applies on Linux too. The switch is
still opt-in.

- [ ] Clear `system_proxy_resolver` for Tor profiles when configuring their
      network context params (all of Win, Mac and Linux), and cover it with a
      Tor browser test

## 2. Upcoming deprecations

### V8 `Data()` → `DataV2()`

`FunctionCallbackInfo::Data()` and `PropertyCallbackInfo::Data()` are
`V8_DEPRECATE_SOON`
([2e0984c6ecef8](https://chromium.googlesource.com/chromium/src/+/2e0984c6ecef8)).
Replace `info.Data()` with `info.DataV2().As<v8::Value>()`.

- [x] `components/brave_wallet/renderer/js_polkadot_provider.cc:34`
- [x] `components/brave_wallet/renderer/js_polkadot_provider.cc:41`

### `raw_ptr` in templated containers

An upcoming clang-plugin update will ban raw `T*` elements in container fields
([69c8260425652](https://chromium.googlesource.com/chromium/src/+/69c8260425652),
go/miracleptr-in-containers). Use `raw_ptr<T>` for the elements. The list below
comes from a heuristic grep, so the plugin may report more.

- [x] `browser/tor/tor_profile_manager.h` `tor_profiles_`
- [x] `browser/ephemeral_storage/application_state_observer.h` `observers_`
- [x] `browser/containers/containers_service_delegate_unittest.cc` `observers_`
- [x] `browser/ui/views/playlist/selectable_list_view.h` `child_views_`
- [x] `browser/ui/views/playlist/selectable_list_view.h` `selected_views_`
- [x] `browser/ui/views/tabs/brave_tab_container.h` `closing_tabs_`
- [x] `browser/ui/views/tabs/tab_style_views_unittest.cc` `split_tabs_`
- [x] `browser/ui/tabs/shared_pinned_tab_service.cc` `dummy_contentses_`
- [x] `browser/ui/tabs/shared_pinned_tab_service.h` `browsers_`
- [x] `browser/ui/tabs/shared_pinned_tab_service.h` `closing_browsers_`
- [x] `browser/ui/tabs/shared_pinned_tab_service.h` `in_tab_dragging_browsers_`
- [x] `browser/permissions/mock_permission_lifetime_prompt_factory.h` `prompts_`
- [x] `chromium_src/components/search_engines/brave_template_url_prepopulate_data_unittest.cc`
      `brave_prepopulated_engines_` (`RAW_PTR_EXCLUSION`: static data exposed as
      a span of raw pointers)
- [x] `components/brave_wallet/browser/json_rpc_service_unittest.cc`
      `eth_call_handlers_`
- [x] `components/brave_wallet/browser/json_rpc_service_unittest.cc`
      `sol_rpc_call_handlers_`
- [x] `components/brave_shields/core/browser/ad_block_filters_provider_manager.h`
      `default_engine_filters_providers_`
- [x] `components/brave_shields/core/browser/ad_block_filters_provider_manager.h`
      `additional_engine_filters_providers_`
- [x] `components/ai_chat/core/browser/associated_content_manager.h`
      `content_delegates_`

### `raw_ptr` checks coming to Blink core

A new plugin flag stops exempting `third_party/blink/renderer/core/`
([e288df60a9cce](https://chromium.googlesource.com/chromium/src/+/e288df60a9cce)),
and upstream turned it on
([01a6f96982aa7](https://chromium.googlesource.com/chromium/src/+/01a6f96982aa7)).
Paths match by substring, so Brave's page graph is covered too.

- [x] `third_party/blink/renderer/core/brave_page_graph/**`: fixed during the
      rebase with `raw_ptr<T, UnprotectedInRelease>`, as upstream does in Blink,
      and `STACK_ALLOCATED()` for `XmlUtf8String`. The dead `FingerprintingRule`
      was removed

### iOS `web::ScriptMessage::legacy_body()`

`legacy_body()` and the `std::unique_ptr<base::Value>` constructor are
deprecated in favour of `ScriptMessageValue` and `body()` (crbug.com/514993435)
([c2614ce5b6e13](https://chromium.googlesource.com/chromium/src/+/c2614ce5b6e13)).
Calling `body()` on a message built with the legacy constructor CHECK-fails, so
migrate the constructors first. Example:
[6bfd6d0a49ece](https://chromium.googlesource.com/chromium/src/+/6bfd6d0a49ece).

Features that reply (`ScriptMessageReceivedWithReply`) can receive messages that
`prompt_facade.mm` builds with the legacy constructor, so they can't use
`body()` until it is migrated: `ScriptMessageValue` has no constructor from
`base::Value`, so it needs converting to an `NSObject` first. The plain
`ScriptMessageReceived` features are done. `playlist` passes the dict on as a
`base::DictValue`, and `protection_stats` iterates a list, which the non-const
`ScriptMessageListValue::begin()` doesn't allow on a `body()`.

- [ ] Legacy constructor: `ios/web/js_messaging/prompt_facade.mm:91`,
      `ios/browser/api/favicon/favicon_driver.mm:142`
- [ ] `ios/browser/api/favicon/favicon_driver.mm:148`, `:156`
- [x] `ios/browser/brave_ads/ads_media_reporting_javascript_feature.mm:53`
- [ ] `ios/browser/brave_search/brave_search_make_default_javascript_feature.mm:74`
- [ ] `ios/browser/brave_shields/protection_stats_javascript_feature.mm:56`
- [ ] `ios/browser/brave_shields/request_blocking/request_blocking_javascript_feature.mm:71`
- [x] `ios/browser/brave_talk/brave_talk_launcher_javascript_feature.mm:63`
- [ ] `ios/browser/playlist/playlist_javascript_feature.mm:125`
- [ ] `ios/browser/skus/skus_javascript_feature.mm:167`
- [ ] `ios/browser/web/de_amp/de_amp_javascript_feature.mm:102`
- [x] `ios/browser/web/document_fetch/document_fetch_javascript_feature.mm:76`
- [ ] `ios/browser/web/logins/logins_javascript_feature.mm:63`
- [ ] `ios/web/js_messaging/message_handler_token.mm:24`
- [ ] `ios/web/js_messaging/prompt_facade_unittest.mm:56`, `:206`

### `base::NotFatalUntil::M138`

Upstream is deleting old `NotFatalUntil` values with
`base/tools/clean-up-not-fatal-until.py`, and removed M136–M151 in this range
([2f77edf82ed8a](https://chromium.googlesource.com/chromium/src/+/2f77edf82ed8a),
[fcfd8f37188d6](https://chromium.googlesource.com/chromium/src/+/fcfd8f37188d6)).
M152 and later are next, planned for November.

- [x] `components/brave_wallet/browser/network_manager.cc:120`: delete the
      `DumpWithoutCrashing` block gated on M138, dead since M139
      (brave-browser#46940): re-armed for M157 on purpose in 0ad3dd94f86
- [ ] `components/brave_wallet/browser/network_manager.cc:121`: delete the block
      once M157 data is in, before upstream removes `M157`
- [ ] `docs/best-practices/coding-standards.md:572`: the `NotFatalUntil::M140`
      example no longer compiles; use a current milestone

### Key pinning split from the HSTS preload list

Pins now build under `chrome_key_pinning_supported` /
`BUILDFLAG(CHROME_KEY_PINNING_SUPPORTED)`
([98c94f7896663](https://chromium.googlesource.com/chromium/src/+/98c94f7896663),
[7c21bfc34ced1](https://chromium.googlesource.com/chromium/src/+/7c21bfc34ced1)).
Both flags default to `!is_cronet_build` today, but a TODO in `net/features.gni`
plans to turn pinning off on iOS, where Brave relies on it.

- [ ] `ios/browser/api/net/BUILD.gn:23`: depend on
      `//net/http:generate_transport_security_state_pins`
- [ ] `ios/browser/api/net/certificate_utility.mm:43`,
      `ios/testing/certificate_unittest.mm:19`,
      `net/http/brave_cert_pinning_test.cc:45` and
      `chromium_src/net/http/transport_security_state.cc:18`: check
      `CHROME_KEY_PINNING_SUPPORTED`
- [ ] When upstream changes the iOS default, set `chrome_key_pinning_supported`
      for Brave iOS

### Flag-gated replacement UIs bypass Brave's overrides

Upstream is building replacements for UIs Brave customizes, behind flags that
are off at 157 but some of which are exposed in about:flags:

- `kAppMenuGlowUp`: `ActionAppMenu`
  ([e406acf9fec52](https://chromium.googlesource.com/chromium/src/+/e406acf9fec52))
  never creates an `AppMenuModel`, so `BraveAppMenuModel` never runs.
- `kWebUIToolbar` and `kWebUIAppMenuButton` construct the plain `AppMenuModel`.
- `kWebUIOmniboxFullPopup`
  ([842c093c08bc6](https://chromium.googlesource.com/chromium/src/+/842c093c08bc6))
  skips Brave's omnibox popup overrides.
- `kWebUILocationBar`, `kWebUIAvatarButton` and `kWebium` replace views Brave
  subclasses.

Brave only pins the two WebUI omnibox flags.

- [ ] Ship these flags disabled with `set_feature_flag_default_state`, and list
      them in `app/feature_defaults_unittest.cc`
- [ ] `browser/ui/views/toolbar/brave_toolbar_view.cc:234`: the comment says
      `kWebUILocationBar` is force-disabled, but nothing overrides it
- [ ] When porting: express Brave's menu items as browser actions.
      `ActionAppMenuManager` uses the upstream `IDS_NEW_INCOGNITO_WINDOW`
      (`action_app_menu_manager.cc:616`), not Brave's "Private window", and
      skips the `IDS_SAVE_AND_SHARE_MENU` substitution in
      `chromium_src/chrome/browser/ui/toolbar/app_menu_model.cc`. This range
      added more entry points: web apps
      ([874b35706ec7e](https://chromium.googlesource.com/chromium/src/+/874b35706ec7ea6d74da93564caccd352fbe3f88))
      and a merged Extensions and Skills submenu
      ([d8198813e39e2](https://chromium.googlesource.com/chromium/src/+/d8198813e39e2742ae461656387d4c6ad47ec9d4))
- [ ] Pin `kSearchSettingsUpdate` and `kSearchSettingsWithMoreEngines` disabled
      (both are in about:flags). They make the search page call the new
      `getDefaultSearchEnginePickerData` handler
      ([207613ffec368](https://chromium.googlesource.com/chromium/src/+/207613ffec368727336d1834534bbbcef1028b15)),
      which skips `BraveSearchEnginesHandler::GetSearchEnginesList()` (the Tor
      search engine filter and the JP ordering), and upstream's own
      `GetSearchEnginesList()` now `CHECK`s that `kSearchSettingsUpdate` is off
      (`search_engines_handler.cc:277`). With `kSearchSettingsWithMoreEngines`
      on, `HandleSetDefaultSearchEngine` also accepts `prepop:` IDs
      ([6ac6e3080c1d5](https://chromium.googlesource.com/chromium/src/+/6ac6e3080c1d554ba9e5270f860b51e4ae361e17)),
      which Brave's copied `ParseTemplateURLId`
      (`browser/ui/webui/settings/brave_search_engines_handler.cc:42`) `CHECK`s
      on

### Android settings in a tab bypass Brave's settings substitutions

Upstream enabled `SettingsInTabUrlNav`
([dc6d4c6952f2d](https://chromium.googlesource.com/chromium/src/+/dc6d4c6952f2d)).
When settings are tab-hosted (`kSettingsInTabDesktop`, on for desktop Android),
navigation goes through `chrome://settings/<route>` and
`SettingsFragmentRegistry`. That skips `BraveSettingsLauncherImpl`, where Brave
swaps in its fragments (downloads, clear browsing data, safe browsing, tabs),
and Brave's XML replacements. Phones and tablets are not affected
(`kSettingsInTab` is off).

- [ ] `android/java/org/chromium/chrome/browser/settings/BraveSettingsLauncherImpl.java:48`:
      don't force `BraveSettingsActivity` on intents built for a settings tab,
      as `BraveSettingsIntentUtil` already avoids
- [ ] Map upstream routes to Brave's fragments (`SettingsFragmentRegistry`
      plaster), or ship `kSettingsInTabDesktop` disabled. This includes
      `PrivacySettings`: clearing browsing data now finishes into it
      ([efa6bfef8bae6](https://chromium.googlesource.com/chromium/src/+/efa6bfef8bae6)),
      which opens upstream's page instead of `BravePrivacySettings`
- [ ] Use `startSettings()` instead of `createSettingsIntent()` +
      `startActivity()`, as in
      [c6cbdc69975ef](https://chromium.googlesource.com/chromium/src/+/c6cbdc69975ef):
  - [ ] `android/java/org/chromium/chrome/browser/shields/ContentFilteringFragment.java:152`,
        `:178`
  - [ ] `android/java/org/chromium/chrome/browser/settings/BraveWalletNetworksPreferenceFragment.java:152`
  - [ ] `android/java/org/chromium/chrome/browser/crypto_wallet/activities/NetworkSelectorActivity.java:145`
  - [ ] `browser/password_manager/android/java/src/org/chromium/chrome/browser/password_manager/BravePasswordManagerHelper.java:58`

### Site settings moving to Lit

Upstream added `SiteSettingsMixinLit`, with its own `expandSiteException()`
([42fc6e1e3423b](https://chromium.googlesource.com/chromium/src/+/42fc6e1e3423b)).
[057732863da77](https://chromium.googlesource.com/chromium/src/+/057732863da77)
moved `add-site-dialog`, `anti-abuse-page`, `category-setting-exceptions`,
`media-picker` and `settings-category-default-radio-group` to it, and Brave has
no Polymer override on those except the add-site dialog (now a lit_mangler,
11dfe54779e). `site-list` and its siblings are still Polymer. When they move,
Brave's `braveCookieType` passthrough is lost, and the Polymer overrides of
those elements stop applying.

- [ ] Add `braveCookieType` to `expandSiteException()` in
      `site_settings_mixin_lit.ts`, as
      `patches/chrome-browser-resources-settings-site_settings-site_settings_mixin.ts.patch`
      does for the Polymer mixin
- [ ] Port to lit_manglers as each element migrates:
      `browser/resources/settings/br/site_list_entry.ts:59`, `br/site_list.ts`,
      `br/site_details.ts` and `br/all_sites.ts`; also drop the now no-op
      `RegisterPolymerComponentToIgnore('add-site-dialog')` at `br/config.ts:17`

### Extension allowlists need SHA-256 hashes

`SimpleFeature::IsIdInList` matches both the SHA-256 and the SHA-1 hashed
extension ID, with a `TODO(crbug.com/455599844)` to drop SHA-1 once
`kUseSha256ForExtensionHashes` is enabled by default. Upstream started appending
SHA-256 hashes to its allowlists
([60ce4a81b5883](https://chromium.googlesource.com/chromium/src/+/60ce4a81b58832810b2e6eb095ece2ff4ca4dd8d),
[8fc1f00e0905a](https://chromium.googlesource.com/chromium/src/+/8fc1f00e0905af1492e57ef8c79267ebe4d46549),
[da26e414c088a](https://chromium.googlesource.com/chromium/src/+/da26e414c088a2bf47171af885e1f5ddaad3721d)).
Brave's entries are SHA-1 only, so Shields, Rewards, Web Discovery and
WebTorrent would lose their APIs. Append the SHA-256 hash next to each one, and
switch the "openssl sha1" recipes in the comments to `sha256`.

- [ ] `common/extensions/api/_api_features.json:18`, `:19`, `:41`, `:42`
- [ ] `chromium_src/chrome/common/extensions/api/_permission_features.json:19`
- [ ] `chromium_src/extensions/common/api/_api_features.json:25`, `:34`, `:43`
- [ ] `chromium_src/extensions/common/api/_manifest_features.json:19`

### `raw_ptr<T, UnprotectedInRelease>` is on its way out

Upstream added the `remove_unprotected_in_release_trait` GN arg (default false)
and an `UnprotectedInReleaseForPerformance` trait for verified hotspots
([8ac71046803ea](https://chromium.googlesource.com/chromium/src/+/8ac71046803ea7b9e38e9d0d07cf59f3bebba1ba),
[c7309abff54d4](https://chromium.googlesource.com/chromium/src/+/c7309abff54d48f2f390c926f0f006da2fba7250)).
Once it flips, every `UnprotectedInRelease` field gets BackupRefPtr protection,
with its cost. Brave used the trait in the rebase for the page graph.

- [ ] `third_party/blink/renderer/core/brave_page_graph/`: 24 uses, in
      `graph_edge.h`, `graph_item.h`, `page_graph.h`, `tracked_request.h`,
      `request_tracker.h`, `script_tracker.h`, `node_html.h`,
      `edge_node_insert.h` and `edge_event_listener_action.h`
      (`git grep -n UnprotectedInRelease -- third_party/blink`). Use plain
      `raw_ptr<T>` unless a field is a measured hotspot

## 3. Code-health migrations

### TabHelpers → TabFeatures

`chrome/browser/ui/tab_helpers.h` now says not to use `TabHelpers` on desktop,
and to prefer `TabFeatures` on Android. Upstream's `TabHelpers` creations fell
from 111 at 155 to 24 at 157.0.8093.0 and 22 at 157.0.8094.1 (counting
`CreateForWebContents(` lines in `tab_helpers.cc`), e.g.
[771fe024274f4](https://chromium.googlesource.com/chromium/src/+/771fe024274f4)
(desktop and Android) and
[50e8515cf41eb](https://chromium.googlesource.com/chromium/src/+/50e8515cf41eb).
The idiom:

- Drop `WebContentsUserData`. Add `DECLARE_USER_DATA(T)`, a
  `ui::ScopedUnownedUserData<T>` member, a `(tabs::TabInterface&, WebContents*)`
  constructor and `static T* From(tabs::TabInterface*)`.
- Desktop: create it in `BraveTabFeatures::Init()` with
  `GetUserDataFactory().CreateInstance<T>(tab, tab, tab.GetContents())`. Desktop
  `TabFeatures` outlive a discard, so the helper must follow the new contents:
  derive it from `tabs::ContentsObservingTabFeature`, as Brave already does for
  `ContainerTabTracker`.
- Android: create it in the `BraveTabFeatures` constructor; `TabFeatures` are
  rebuilt per `WebContents`.
- Callers: `T::From(tabs::TabInterface::MaybeGetFromContents(web_contents))`.

Brave already has `BraveTabFeatures` on desktop
(`browser/ui/tabs/brave_tab_features.cc`) and Android
(`browser/android/brave_tab_features.cc`). Move the helpers created in
`browser/brave_tab_helpers.cc`:

- [ ] `YouTubeScriptInjectorTabHelper`
- [ ] `brave_shields::BraveShieldsTabHelper`
- [ ] `BraveGeolocationPermissionTabHelper`
- [x] `BackgroundColorTabHelper` (47fec00701c)
- [ ] `brave_rewards::RewardsTabHelper`
- [ ] `ai_chat::AIChatTabHelper`
- [ ] `BraveDrmTabHelper`
- [ ] `BraveWaybackMachineTabHelper`
- [ ] `brave_perf_predictor::PerfPredictorTabHelper`
- [ ] `serp_metrics::SerpMetricsTabHelper`
- [ ] `brave_ads::AdsTabHelper`
- [ ] `brave_ads::CreativeSearchResultAdTabHelper`
- [ ] `web_discovery::WebDiscoveryTabHelper`
- [ ] `speedreader::SpeedreaderTabHelper`
- [ ] `tor::TorTabHelper`
- [ ] `tor::OnionLocationTabHelper`
- [ ] `BraveNewsTabHelper`
- [ ] `OnboardingTabHelper`
- [x] `sidebar::SidebarTabHelper` (9d9c0edd295)
- [ ] `brave_wallet::BraveWalletTabHelper`
- [x] `misc_metrics::PageMetricsTabHelper` (24f49a545bb)
- [x] `RequestOTRTabHelper` (b687378edfa)
- [ ] `playlist::PlaylistTabHelper`
- [x] `content_settings::PageSpecificContentSettings`: skipped, it's upstream's
      own class and upstream `TabHelpers` still creates it
- [x] `brave_shields::BraveShieldsWebContentsObserver`: skipped, it's also
      attached to non-tab `WebContents` (AI Chat, offliner, presentation
      receiver, backup search results)
- [x] `ephemeral_storage::EphemeralStorageTabHelper`: skipped, same reason
- [x] `browser/ui/tabs/brave_tab_features.cc`: make `TabDataWebContentsObserver`
      and `WebMcpInjector` follow discards (`ContentsObservingTabFeature`); they
      kept observing the old `WebContents` (ca20565f3a3)

### `crypto/sha2.h` and `crypto/secure_hash.h` → `crypto/hash.h`

Both headers are deprecated and being removed (crbug.com/374310081). Upstream
example: [ecdf557d](https://chromium.googlesource.com/chromium/src/+/ecdf557d),
which replaces `crypto::kSHA256Length` with `crypto::hash::kSha256Size`.

- [x] `browser/extensions/brave_crx_generation_browsertest.cc`
- [x] `components/brave_component_updater/browser/brave_component_installer.cc`
- [x] `components/brave_rewards/core/engine/hash_prefix_store.cc`
- [x] `components/brave_rewards/core/engine/hash_prefix_store_unittest.cc`
- [x] `components/brave_rewards/core/engine/publisher/prefix_util.cc`
- [x] `components/brave_rewards/core/engine/util/random_util.cc`
- [x] `components/brave_rewards/core/engine/util/request_signer.cc`
- [x] `components/brave_rewards/core/engine/wallet_provider/bitflyer/connect_bitflyer_wallet.cc`
- [x] `components/brave_service_keys/brave_service_key_utils.cc`
- [x] `components/brave_shields/content/browser/ad_block_subscription_service_manager.cc`
- [x] `components/brave_shields/core/browser/ad_block_component_installer.cc`
- [x] `components/brave_wallet/browser/wallet_data_files_installer.cc`
- [x] `components/local_ai/core/local_models_updater.cc`
- [x] `components/local_ai/core/on_device_speech_models_component_installer.cc`
- [x] `components/ntp_background_images/browser/ntp_background_images_component_installer.h`
- [x] `components/ntp_background_images/browser/sponsored_content/ntp_sponsored_images_component_installer.h`
- [x] `components/p3a/nitro_utils/cose.cc`
- [x] `components/playlist/content/browser/media_detector_component_installer.cc`
- [x] `components/psst/core/browser/psst_component_installer.cc`
- [x] `components/speedreader/speedreader_rewriter_service.cc`
- [x] `components/web_discovery/browser/background_credential_helper.cc`
- [x] `components/web_discovery/browser/ecdh_aes.cc`
- [x] `components/web_discovery/browser/reporter.cc`
- [x] `components/web_discovery/browser/signature_basename.cc`
- [x] `components/web_discovery/browser/signature_basename_unittest.cc`
- [x] `components/web_mcp/core/browser/web_mcp_component_installer.cc`
- [x] `ios/browser/api/certificate/models/brave_certificate_fingerprint.mm`
- [x] `net/http/partitioned_host_state_map.cc`
- [x] `net/http/partitioned_host_state_map.h`
- [x] `net/http/partitioned_host_state_map_unittest.cc`

### Globals with exit-time destructors

Upstream keeps replacing non-trivial globals (e.g. `std::string` constants) with
trivially destructible ones, and removed the warning opt-out from 103 more
directories in this range
([f441240bbd533](https://chromium.googlesource.com/chromium/src/+/f441240bbd533)).
In Brave the remaining debt is its `[[clang::no_destroy]]` globals. The idioms
are `constexpr char[]` or `std::array<const char*>` for strings
([5f306ea932644](https://chromium.googlesource.com/chromium/src/+/5f306ea932644)),
constexpr `base::MakeFixedFlatMap` for maps
([982b66c8dad34](https://chromium.googlesource.com/chromium/src/+/982b66c8dad34)),
`static constexpr re2::LazyRE2` for patterns
([a8341bafd1de1](https://chromium.googlesource.com/chromium/src/+/a8341bafd1de1)),
and a function-local `base::NoDestructor` otherwise.

- [x] `components/tor/tor_control_event.h`: `kTorControlEventByName` → constexpr
      `base::fixed_flat_map`; `kTorControlEventByEnum` → `operator<<` for
      `TorControlEvent` (used via `base::ToString`)
- [ ] Constant tables:
  - [ ] `components/brave_perf_predictor/browser/bandwidth_linreg_parameters.h:264`,
        `:485`, `:682`, `:694` (in a header, so one static initializer per
        includer)
  - [ ] `components/brave_private_cdn/headers.h:19`
  - [ ] `components/brave_wallet/browser/solana_instruction_builder.h:70`
  - [ ] `components/brave_rewards/core/engine/endpoints/common/post_create_transaction.h:35`
  - [ ] `components/brave_news/browser/feed_building.cc:48`, `:68`
  - [ ] `components/ntp_tiles/brave_popular_sites_impl.cc:15`
  - [ ] `build/ios/mojom/cpp_transformations.h:21`, `:43`
- [ ] RE2 patterns: `components/brave_news/browser/html_parsing.cc:57`, `:68`,
      `:85`, `:103`
- [ ] Function-local statics:
  - [ ] `components/brave_rewards/content/rewards_protocol_navigation_throttle.cc:128`
  - [ ] `components/brave_rewards/content/rewards_service_impl.cc:529`, `:557`
  - [ ] `components/brave_rewards/core/engine/util/rewards_prefs.cc:78`
  - [ ] `components/brave_wallet/browser/bitcoin/bitcoin_serializer.cc:39`,
        `:63`, `:76`
  - [ ] `browser/ui/views/brave_help_bubble/brave_help_bubble_host_view.cc:49`
  - [ ] `browser/ui/views/brave_tooltips/brave_tooltip_popup_handler.cc:21`
- [ ] Test-override globals (use a function-local `base::NoDestructor`
      accessor):
  - [ ] `browser/extensions/api/identity/brave_web_auth_flow.cc:36`
  - [ ] `components/brave_wallet/browser/asset_ratio_service.cc:139`
  - [ ] `components/brave_wallet/browser/wallet_data_files_installer.cc:55`
  - [ ] `components/brave_search/browser/brave_search_fallback_host.cc:19`
  - [ ] `components/brave_referrals/browser/brave_referrals_service.cc:87`
- [ ] Tests:
  - [ ] `app/brave_main_delegate_browsertest.cc:61`
  - [ ] `components/l10n/common/ofac_sanction_util_unittest.cc:34`
  - [ ] `components/brave_shields/content/test/csp_merge_unittest.cc:20`, `:26`
  - [ ] `components/ntp_background_images/browser/view_counter_model_unittest.cc:28`
  - [ ] `components/brave_wallet/browser/blockchain_registry_unittest.cc:231`,
        `:255`, `:279`, `:304`

### Feature getters → `UnownedUserData`

Upstream removed every public getter from `BrowserWindowFeatures`, and both it
and `TabFeatures` now say "Do not add more public accessors"
(crbug.com/481268779)
([99f9595406318](https://chromium.googlesource.com/chromium/src/+/99f9595406318),
[6126eaab5a9f6](https://chromium.googlesource.com/chromium/src/+/6126eaab5a9f6),
[ffd23d0098426](https://chromium.googlesource.com/chromium/src/+/ffd23d0098426)).
Add `DECLARE_USER_DATA`, a `ScopedUnownedUserData` member and a static
`From(BrowserWindowInterface*)` or `From(tabs::TabInterface*)`, then delete the
getter. Brave already does this for 14 classes, e.g. `BraveVPNController`.

- [x] `browser/ui/browser_window/public/browser_window_features.h:59`
      `sidebar_controller()` (34 call sites in 22 files) (60e1ca8a532)
- [x] `browser/ui/browser_window/public/browser_window_features.h:63`, `:67`
      `focus_mode_controller()` (42 call sites in 14 files) (c2a6e9ef71a)
- [ ] `browser/ui/tabs/public/brave_tab_features.h:80`, `:83`, `:90`, `:97`,
      `:104`, `:111`, `:118`, `:125`: PSST, partitioned storage, Speedreader,
      Wayback, Playlist, onion location and Brave News getters, reached through
      `BraveTabFeatures::FromTabFeatures()`

### NullAway: Brave Java without `@NullMarked`

Upstream keeps annotating its classes, e.g. `ChromeActivity` and
`ChromeTabbedActivity`
([3223b5d0026a1](https://chromium.googlesource.com/chromium/src/+/3223b5d0026a1),
[38373c2e8ddf0](https://chromium.googlesource.com/chromium/src/+/38373c2e8ddf0)).
421 of 704 Brave production Java files have no `@NullMarked`, against 47 of 1495
upstream in `chrome/android/java/src`. 90 Brave subclasses are unannotated while
their parent is `@NullMarked`, so their overrides aren't checked. Presubmit only
forces the annotation on new files. List the files with
`git ls-files '*.java' | grep -vE '/(javatests|junit|test)/|Test\.java$|build/android/bytecode' | xargs grep -L '@NullMarked'`.

- [ ] `@NullMarked` files that still use androidx `@Nullable`/`@NonNull`, which
      can't annotate type arguments (what broke in the rebase fix for
      `@Nullable` suppliers). Switch to `org.chromium.build.annotations`:
  - [ ] `android/java/org/chromium/base/BraveCommandLineInitUtil.java`
  - [ ] `android/java/org/chromium/chrome/browser/crypto_wallet/util/WalletUtils.java`
  - [ ] `android/java/org/chromium/chrome/browser/crypto_wallet/util/AndroidUtils.java`
  - [ ] `android/java/org/chromium/chrome/browser/firstrun/WelcomeOnboardingActivity.java`
  - [ ] `android/java/org/chromium/chrome/browser/homepage/settings/BraveRadioButtonGroupHomepagePreference.java`
  - [ ] `android/java/org/chromium/chrome/browser/homepage/settings/BraveRadioButtonGroupHomepagePreferenceDummySuper.java`
  - [ ] `android/java/org/chromium/chrome/browser/media/BraveYouTubePictureInPictureController.java`
  - [ ] `android/java/org/chromium/chrome/browser/omnibox/suggestions/BraveAutocompleteMediator.java`
  - [ ] `android/java/org/chromium/chrome/browser/settings/BraveShredPreference.java`
  - [ ] `android/java/org/chromium/chrome/browser/settings/BraveShredPreferencesFragment.java`
  - [ ] `android/java/org/chromium/chrome/browser/tabbed_mode/BraveTabbedAppMenuPropertiesDelegate.java`
  - [ ] `android/java/org/chromium/chrome/browser/ui/BraveAdaptiveToolbarUiCoordinator.java`
  - [ ] `android/java/org/chromium/chrome/browser/vpn/adapters/AlwaysOnPagerAdapter.java`
  - [ ] `browser/customize_menu/android/java/src/org/chromium/brave/browser/customize_menu/CustomizeBraveMenu.java`
  - [ ] `browser/password_manager/android/test_support/java/src/org/chromium/chrome/browser/password_manager/FakePasswordManagerHandler.java`
- [ ] Annotate the unannotated subclasses of `@NullMarked` parents, starting
      with `BraveActivity`

### `base::Reversed` → `std::views::reverse`

Upstream replaced most `base::Reversed` uses (327 down to 53;
[b6ae90def7daa](https://chromium.googlesource.com/chromium/src/+/b6ae90def7daaa1d117ca983bf05dcf5e319d143)),
though the header isn't deprecated yet. It doesn't work on containers that
aren't bidirectional ranges.

- [ ] `browser/ui/tabs/brave_tree_tab_strip_collection_delegate.cc:1680`

### `DeprecatedCreateOwnedBrowserWindowForTesting`

`create_browser_window.h` now says the helper is deprecated until unit tests
stop needing a `Browser`. Upstream's uses fell from 12 to 8 in this range, e.g.
[99915352f4ebd](https://chromium.googlesource.com/chromium/src/+/99915352f4ebde2cc8f086ae28a9b4e8dc5ef4b2).
The replacements are `TestWebContentsFactory` or `MockBrowserWindowInterface`.

- [ ] `browser/download/bubble/download_display_controller_unittest.cc:337`
- [ ] `browser/renderer_context_menu/test/render_view_context_menu_unittest.cc:110`
- [ ] `browser/ui/views/toolbar/brave_vpn_button_unittest.cc:54`

### Android: stop mocking `View` and `Activity`

An ErrorProne `DoNotMock` check is rolling out (crbug.com/567604165); upstream's
mocks fell from 377 to 306
([1a5d42fa87876](https://chromium.googlesource.com/chromium/src/+/1a5d42fa87876d64f173ced94b27df5c32c36cef)).
Use `new View()`, `FrameLayout` or `Robolectric.buildActivity()`. Upstream's
suppressions fell from 89 to 70 in 157.0.8094.1. Brave suppresses the check with
`@SuppressWarnings("DoNotMock")` in eight tests that still mock:

- [ ] `android/javatests/…/BraveMediaSessionHelperTest.java`
- [ ] `…/BraveNewBackgroundTabAnimationDataUnitTest.java`
- [ ] `…/BraveFullscreenHtmlApiHandlerBaseTest.java`
- [ ] `…/BraveFullscreenVideoPictureInPictureControllerTest.java`
- [ ] `…/BraveYouTubePictureInPictureControllerTest.java`
- [ ] `…/BraveSwipeRefreshHandlerTest.java`
- [ ] `…/hub/BraveHubToolbarViewUnitTest.java`
- [ ] `…/ui/system/BraveStatusBarColorControllerUnitTest.java`
      (`git grep -l 'SuppressWarnings("DoNotMock")' -- android`)

### iOS: `TypedProfileKeyedServiceFactoryIOS`

Upstream is converting iOS keyed service factories to
`class F final : public TypedProfileKeyedServiceFactoryIOS<F, Service>`, with a
public `F(PassKey)` constructor and only `BuildServiceInstanceFor`, instead of
hand-written `GetInstance()`/`GetForProfile()` and a `NoDestructor` friend
(crbug.com/570433691;
[5e0a42bf2c320](https://chromium.googlesource.com/chromium/src/+/5e0a42bf2c32063b90123ed9e9108dc1abdcbe95),
e.g.
[863978bcc204f](https://chromium.googlesource.com/chromium/src/+/863978bcc204f10286ae8f6a5a53de37266464d1);
refcounted:
[fc8bb7844103f](https://chromium.googlesource.com/chromium/src/+/fc8bb7844103f7c80f7967e2bf67246aa1c77da1)).
Upstream files on the typed base went from 1 to 12 in 157.0.8094.1
(`git grep -lE 'public Typed(Refcounted)?ProfileKeyedServiceFactoryIOS\b' <tag> -- ios`).

- [ ] Brave's 15 factories:
      `git grep -lE 'public (Refcounted)?ProfileKeyedServiceFactoryIOS\b' -- ios`
      (AI Chat ×3, webcompat reporter, Brave Account, ads, Origin, Shields
      settings, wallet, debounce, favicon loader, misc metrics, SERP metrics,
      SKUs, URL sanitizer)

## 4. Obsolete Brave code

### Fledge and AdInterestGroupAPI overrides

Upstream disabled both by default
([7270d1697cac1](https://chromium.googlesource.com/chromium/src/+/7270d1697cac1)),
so Brave's overrides now match upstream. Keep the origin-trial block, since
AdInterestGroupAPI still has a trial name.

- [ ] `rewrite/third_party/blink/renderer/platform/runtime_enabled_features.json5.yaml:70`,
      `:115`, and `EnableFledge(false)` in
      `renderer/brave_content_renderer_client.cc:126`, or keep them deliberately
      as pins (keep the tests either way)
- [ ] Same for fenced frames: `ResolveInvalidConfigurations()` now disables the
      runtime feature whenever `blink::features::kFencedFrames` is off, which
      Brave pins
      ([5625011527550](https://chromium.googlesource.com/chromium/src/+/5625011527550c51a1745b863b197788e23bc923)).
      `EnableFencedFrames(false)` at `brave_content_renderer_client.cc:128` and
      the `FencedFramesRuntime` plaster
      (`runtime_enabled_features.json5.yaml:56`, `:120`) are now redundant

### Leftover `prefs` binding on the Brave Origin page

The page is Lit now and has no `prefs` property.

- [x] `browser/resources/settings/br/settings_main.ts:84`: drop
      `prefs="{{prefs}}"` (ac89f39f765)

### Widevine headers dep added twice

Upstream added the `//third_party/widevine/cdm:headers` dep to
`//chrome/common:unit_tests`
([c57c951e07bda](https://chromium.googlesource.com/chromium/src/+/c57c951e07bda))
a day before Brave's plaster for it landed, so the generated patch now adds a
second identical line.

- [ ] `rewrite/chrome/common/BUILD.gn.yaml:57`: drop the substitution and
      regenerate the patch

### Linux EULA plaster is a no-op

Upstream now requires the EULA only in Chrome-branded builds
([8ee32546454ad](https://chromium.googlesource.com/chromium/src/+/8ee32546454ad)).
Brave's plaster still matches, but only rewrites the branded branch, which Brave
never compiles.

- [ ] Delete `rewrite/chrome/browser/first_run/first_run.h.yaml` and its patch

### Workaround for the removed `CheckSettingsChanges` presubmit

Upstream removed the check in favour of `SearchIndexValidator`
([54bec2ab4cdf8](https://chromium.googlesource.com/chromium/src/+/54bec2ab4cdf8)).

- [ ] `android/javatests/org/chromium/chrome/browser/settings/BraveMainSettingsFragmentTest.java:272`:
      rejoin the split `SEARCH_INDEX_DATA_PROVIDER` string and drop the comment
- [ ] `docs/android_settings_search_index.md:183`: describe
      `SearchIndexValidator` instead of the presubmit

### `//chrome/browser/ui:ui` is now an empty aggregator

Upstream emptied it: no sources, deps or `allow_circular_includes_from`, and a
comment saying not to add any
([bd01150165f8e](https://chromium.googlesource.com/chromium/src/+/bd01150165f8ef86f2bf3ade53ac6d7281684038),
[4fdcb77e01c1f](https://chromium.googlesource.com/chromium/src/+/4fdcb77e01c1ff4699a09f1265be5633aedb338a),
[92988b38e867f](https://chromium.googlesource.com/chromium/src/+/92988b38e867f8c978a84f1ab847662ccc5dac3f),
[5e4566e68f3d0](https://chromium.googlesource.com/chromium/src/+/5e4566e68f3d0bff87a4a3c75e6cb7680566c571)).
`patches/chrome-browser-ui-BUILD.gn.patch` still adds to it, so its
`allow_circular_includes_from` is a no-op and the deps are added after the lock.
Verify with `gn check`.

- [ ] Drop the `allow_circular_includes_from` hunk (`:25`) and move
      `brave_chrome_browser_ui_deps` into `brave_chrome_browser_deps`
- [ ] Drop `brave_chrome_browser_ui_allow_circular_includes_from`
      (`browser/sources.gni:859`) if nothing else uses it; `//brave/browser/ui`
      is already in `brave_chrome_browser_allow_circular_includes_from`
- [ ] Check that no Brave include now needs a circular allowance for the new
      `//chrome/browser/ui/views/frame:browser_view` target

### `UnbufferedFileWriterTest.VeryLarge` override matches nothing

Upstream now builds `VeryLarge` only under `ARCH_CPU_64_BITS`
([fe096b64b0283](https://chromium.googlesource.com/chromium/src/+/fe096b64b02838c561933bc8465ca521c91a4b3c)),
so Brave's `#define VeryLarge DISABLED_VeryLarge` under `ARCH_CPU_X86` renames
nothing.

- [ ] Delete
      `chromium_src/chrome/installer/util/unbuffered_file_writer_unittest.cc`

### Test filters that stopped matching

- [ ] `test/filters/browser_tests.filter:714-716`: delete; upstream removed the
      ReduceAcceptLanguage policy and
      `ReduceAcceptLanguageEnterprisePolicyBrowserTest`
      ([eefdf5ee43d27](https://chromium.googlesource.com/chromium/src/+/eefdf5ee43d273c5aace12b3695856a0ba41b4f2))

- [ ] `test/filters/browser_tests.filter:1244`: delete; upstream deleted
      `GlicEnablingGeminiEntBrowserTest`
      ([bec0d8d11d3c7](https://chromium.googlesource.com/chromium/src/+/bec0d8d11d3c7))
- [ ] Rerun `tools/chromium_tests_analysis/update-upstream-flake-filters.py`:
      `PaymentHandlerWebFlowViewCameraTest` is no longer parameterised
      ([889d7a55dc08e](https://chromium.googlesource.com/chromium/src/+/889d7a55dc08e)),
      so the `All/…/1` entries in
      `test/filters/generated/browser_tests-linux-msan.filter:120` and
      `browser_tests-linux-ubsan.filter:30` don't match, and
      `browser_tests-win-asan.filter:399` names a deleted test. Likewise
      `unit_tests-linux-asan.filter:303`
      `GlicSelectionObserverTest.ShakeTriggerDisabledByPref` is now
      `ShakeTriggerTest.ShakeTriggerDisabledByPref`
      ([1a5815e692a9b](https://chromium.googlesource.com/chromium/src/+/1a5815e692a9b))

- [x] `test/filters/unit_tests.filter:344-346`: upstream parameterised
      `EnclaveAuthenticatorRequestDelegateTest`
      ([8f0c99f9f2b21](https://chromium.googlesource.com/chromium/src/+/8f0c99f9f2b21)),
      and an exact negative filter doesn't match `All/…/0`, so these tests run
      again. Use
      `-All/EnclaveAuthenticatorRequestDelegateTest.BrowserProvidedPasskeysAvailable*/*`:
      already done in 784011a5b45 (`unit_tests.filter:328-330`)
- [ ] `browser_tests.filter:94`: delete.
      `WebUIToolbarWebViewBrowserTest.DropSearchTextOnToolbar` became
      `WebUIToolbarDropBrowserTest.DropPlainText_FromWebPage` and
      `.DropPlainText_FromOs`
      ([42e274290139a](https://chromium.googlesource.com/chromium/src/+/42e274290139a)),
      which still expect a Google search. c0493905fe0 already filters
      `-WebUIToolbarDropBrowserTest.*` at `:2391`
- [ ] `browser_tests.filter:2240-2241`: delete; the renamed tests are already
      filtered at `:2242-2244`
      ([28516df78f9e7](https://chromium.googlesource.com/chromium/src/+/28516df78f9e7))

- [ ] `test/filters/net_unittests.filter:64`, `:65`, `:79`, `:80`: upstream
      replaced
      `TransportSecurityStateTest.DecodePreloaded{Single,MultipleMix,MultiplePrefix}`
      with `PreloadedHsts`
      ([8a12a47926c99](https://chromium.googlesource.com/chromium/src/+/8a12a47926c99)),
      and renamed `PinValidationWithoutRejectedCerts` to
      `PinValidationWithCompiledInPins`
      ([bcb49f457fbff](https://chromium.googlesource.com/chromium/src/+/bcb49f457fbff)).
      The new pin test reads the unittest pins, which Brave's `ParsePkpJson`
      plaster replaces: c0493905fe0 filters it at `:24`. Filter `PreloadedHsts`
      only if CI fails

- [ ] `test/filters/browser_tests.filter:2600-2604`: upstream parameterised five
      tab sharing suites
      ([2442df769ae7e](https://chromium.googlesource.com/chromium/src/+/2442df769ae7e12acbd88d868968610747872e8f),
      `All/…/MigratedInfobar` and `LegacyInfobar`), so the negative filters no
      longer match and the tests run again. Use
      `-All/MultipleTabSharingUIViewsBrowserTest.*/*`,
      `-All/TabSharingMessageLinksBrowserTest.ClickingOn*/*`,
      `-All/TabSharingUIViewsBrowserTest.*/*`,
      `-All/TabSharingUIViewsDataProtectionBrowserTest.*/*` and
      `-All/TabSharingUIViewsPreferCurrentTabBrowserTest.*/*`. The generated
      entries for the same suites are in `browser_tests-linux-asan.filter`
      (`:1479`, `:1482`, `:1485`, `:1488`, `:2433`, `:2436`, `:2439`) and
      `browser_tests-win-asan.filter:510`
- [ ] `browser_tests.filter:2709`: upstream replaced the 23
      `PinnedToolbarActionsContainerTest` tests with
      `PinnedToolbarActionsContainerBrowserTest` and
      `PinnedToolbarActionsContainerWebUIDisabledBrowserTest`
      ([0548d35598e77](https://chromium.googlesource.com/chromium/src/+/0548d35598e77525fc59ff13dd41479d781018c8)).
      Retarget the filter to both (`pinned_toolbar_actions_container_` is still
      null in `BraveToolbarView`), and delete `unit_tests.filter:97`, which
      names a browser test
- [ ] Delete the tests removed with the speculative ML predictors
      ([50ff1e7145202](https://chromium.googlesource.com/chromium/src/+/50ff1e7145202a71a6e1017ab6df897279c5ef05)):
      `browser_tests.filter:2133` `PreloadingModelKeyedServiceTest.Score` (keep
      the `LanguageDetectionModelServiceBrowserTest.*` line its comment is
      shared with) and `unit_tests.filter:492`, `:493`
      `NavigationPredictorUserInteractionsTest.MLModelMaxHoverTime` and
      `.ProcessPointerEventUsingMLModel`
- [ ] Add to the generated-filter rerun above: `unit_tests-linux.filter:996`,
      `:999` and `unit_tests-win.filter:204`, `:207`, `:210` (tests moved from
      `RevokedPermissionsServiceBackfillTest` to
      `UnusedSitePermissionsManagerBackfillTest`,
      [1afdf8f3bb926](https://chromium.googlesource.com/chromium/src/+/1afdf8f3bb926d2f22bc79e66c96d9bb2d0e61ad)),
      `browser_tests-linux.filter:3831`, `:3840`
      (`PageContextMonitorBrowserTest.GetPageContext` and `.LoadStopped` are
      gone,
      [149eae0dcba1f](https://chromium.googlesource.com/chromium/src/+/149eae0dcba1fa48b13edb0f10bc092acd0f7032))
      and `unit_tests-linux-asan.filter:273`, `:285`, `:288`
      (`GlicSelectionObserverTest.CopyLinkToHighlight` and `OnLinkGenerated*`
      are gone,
      [be760df248b9c](https://chromium.googlesource.com/chromium/src/+/be760df248b9cb96ef21293491636135e5947607))

### Pre-existing: stale overrides

- [ ] Polymer overrides of elements that no longer exist upstream:
      `browser/resources/settings/br/basic_page.ts:52`,
      `br/settings_basic_page.ts:10`, `br/printing_page.ts:8`,
      `br/autofill_section.ts:10`, and `br/passwords_section.ts:8`
      (`#checkPasswordsLinkRow` is gone)
- [ ] `browser/resources/settings/br/settings_ui.ts:135`: listens for
      `showing-section`, which upstream no longer dispatches. Handle it with the
      `settings-ui` Lit port in "Settings menu, layout and autofill overrides no
      longer apply"
- [ ] `browser/resources/settings/br/config.ts:18`:
      `RegisterPolymerComponentToIgnore('settings-search-page')`; the element is
      Lit and has a lit_mangler
- [ ] `base/compile_overridden_features.inc`: `PlusAddressesEnabled`,
      `VerticalTabsLaunch`, `CommerceDeveloper`, `EnableForceDownloadToOneDrive`
      and `ReadIsSubjectToUniversalOptOutCapability` aren't features upstream
- [ ] `chromium_src/third_party/blink/common/features.cc:62`:
      `IsPrerender2Enabled()` has no callers
- [ ] `chromium_src/third_party/blink/common/origin_trials/origin_trials.cc:26`:
      `DeviceAttributes`, `InterestCohortAPI`, `FencedFrames`, `Fledge`,
      `SignedExchangeSubresourcePrefetch` and `SubresourceWebBundles` aren't
      trial names upstream
- [ ] Android bytecode hooks whose targets don't exist upstream, none of them
      covered by `BytecodeTest`:
  - [ ] `build/android/bytecode/java/org/brave/bytecode/BraveBookmarkToolbarClassAdapter.java:20`
        (`mBookmarkModel`)
  - [ ] `build/android/bytecode/java/org/brave/bytecode/BraveBookmarkUtilsClassAdapter.java:31`
        (`isSpecialFolder`)
  - [ ] `build/android/bytecode/java/org/brave/bytecode/BraveToolbarManagerClassAdapter.java:66`
        (`mOverlayPanelVisibilitySupplier`)
  - [ ] `build/android/bytecode/java/org/brave/bytecode/BraveTabbedActivityClassAdapter.java:43`
        (`supportsDynamicColors`)
  - [ ] `build/android/bytecode/java/org/brave/bytecode/BraveToolbarLayoutClassAdapter.java:26`
        (`onHomeButtonUpdate`)
- [ ] `components/cached_flags/android/java/src/org/chromium/components/cached_flags/BraveCachedFlag.java:15`:
      six flag names that no longer exist upstream, and `FeedContainment`, which
      isn't a cached flag
- [ ] Test filter entries that match nothing, from earlier lifts: about 8 exact
      names for tests that are now parameterised (e.g.
      `test/filters/browser_tests.filter:463`), 23 `Prefix/…/0` names for tests
      that no longer are (e.g. `browser_tests.filter:546-549`), and about 200
      for deleted tests, mostly Privacy Sandbox. Verify each one;
      `tools/cr/prune_test_filters.py` lists them from built test binaries
- [ ] `test/filters/browser_tests.filter:893-904` and `:2501-2507`: the comments
      wait for CLs that landed long ago; re-evaluate
      `CookieUseCounterBrowserTest.*`
- [ ] `build/chromium/resources/chrome/app/theme/default_{100,200}_percent/common/save_{address,card,password}{,_dark}.png`:
      upstream deleted the originals
      ([0ba8e41f707d5](https://chromium.googlesource.com/chromium/src/+/0ba8e41f707d5)),
      and `branding.js` still copies them in
- [ ] `chromium_src/chrome/browser/preloading/prerender/prerender_manager.h:59`,
      `:61`, `:69`, `:72` (and the `.cc` definitions at `:42`-`:56`): Brave's
      replacement still declares `Start`/`StopPrerenderBookmark` and
      `Start`/`StopPrerenderNewTabPage`, which upstream no longer has or calls
- [ ] `chromium_src/chrome/common/url_constants.h` replaces the upstream header
      outright, and still defines constants upstream deleted (e.g.
      `kGoogleChromeURLScheme`, unused)
- [x] Feature overrides that equal upstream's default (40 at all three tags,
      e.g. `kShoppingList`): kept. `rewriters.pyl` allows them as pins against
      upstream sliding back

## 5. Optional

- [x] Evaluate `base::ElapsedNoSleepTimer`
      ([592bab9770f88](https://chromium.googlesource.com/chromium/src/+/592bab9770f88))
      for Brave duration metrics (P3A, ads, rewards) that system sleep currently
      inflates. Skipped: the only candidates are UMA histograms
      (`Brave.ShieldsCNAMEBlocking.TotalResolutionTime`,
      `Brave.ProxyingURLLoader.TotalRequestTime`), which Brave doesn't upload,
      and upstream has no adopters yet. Revisit once upstream uses it.
- [ ] Pin `kPreloadingModerateViewportHeuristics` off next to the eager one, now
      that upstream enables both everywhere
      ([0b4834a209fb2](https://chromium.googlesource.com/chromium/src/+/0b4834a209fb2)).
      Brave's network prediction pref already stops preloading.
- [ ] Skip picture-in-picture while projected to Android Auto, as upstream does
      ([12df1c62a0ef8](https://chromium.googlesource.com/chromium/src/+/12df1c62a0ef8)).
      Upstream moved the check into `PictureInPicture.isEnabled()`, so
      `BraveYouTubeScriptInjectorNativeHelper.java:55` and
      `BraveToolbarLayoutImpl.java:511`, `:639`, `:1130` now have it. Only
      `BraveYouTubePictureInPictureController.java:673`, which asks the native
      availability check, is left to verify.
- [ ] Use `CHECK` where upstream converted the `DCHECK`s next to Brave code
      ([8758e8b9a6e53](https://chromium.googlesource.com/chromium/src/+/8758e8b9a6e53),
      [15c3cd05b20e6](https://chromium.googlesource.com/chromium/src/+/15c3cd05b20e6)):
      `chromium_src/chrome/browser/bookmarks/android/bookmark_bridge.cc:125`,
      `:146`, `:193`, `:195`, and
      `rewrite/chrome/browser/component_updater/widevine_cdm_component_installer.cc.yaml:33`.
- [ ] Re-sync `tools/android/checkstyle/brave-style-5.0.xml` with upstream: the
      checkstyle 14.1 roll changed FallThrough's `reliefPattern` from `.*`,
      which silently disabled the check
      ([d9283fc6c6d69](https://chromium.googlesource.com/chromium/src/+/d9283fc6c6d69)).
- [ ] Drop `browser_tests.filter:357`
      `WebAccessibleResourcesBrowserTest.DNRRedirectWithQueryAndRef` after a CI
      run: upstream fixed a race in it
      ([5937afcdd2e43](https://chromium.googlesource.com/chromium/src/+/5937afcdd2e43)).
- [ ] Batch `BraveManageSyncSettingsTest`, as upstream now does for
      `ManageSyncSettingsTest`
      ([ac6815a31f32f](https://chromium.googlesource.com/chromium/src/+/ac6815a31f32f)).
      Its `@DoNotBatch` reason is copied and wrong (it doesn't use
      `SyncTestRule`). First reset `sIsChromeOSForTesting`
      (`BraveManageSyncSettings.java:149`) with `ResettersForTesting`.
- [ ] Remove the stale `class Browser;` forward declarations in
      `browser/ui/brave_pages.h:15` and
      `browser/ui/tabs/shared_pinned_tab_service.h:23`.

- [ ] Hide the Android "Switch to Private" sync settings row, as
      `BraveManageSyncSettings.java:100` does for other Google rows
      (`removePreferenceByKey(PREF_SWITCH_TO_INCOGNITO)`), or keep it on desktop
      form factors: `kSwitchToIncognitoInSettings` is now enabled
      ([6e3f842ba7332](https://chromium.googlesource.com/chromium/src/+/6e3f842ba7332ffe2b0fb5060bffe085ef2fc4f0)).
      Its string is a mechanical rebrand
      (`android_chrome_strings_override.grd:146`).
- [ ] Decide whether to pin `kExtensionProtocolHandlers` off: extensions can now
      register protocol handlers
      ([b47bdaeb9682a](https://chromium.googlesource.com/chromium/src/+/b47bdaeb9682aaf58c58e2763e0f962696e7517f)).
      Brave ships no such extension, but its own scheme handling and protocol
      handler UI don't know about them.
- [ ] Decide whether Brave's content agent should be allowed to open or switch
      to the NTP. Upstream moved the built-in NTP exception out of
      `SwitchTabTool` and `OpenKnownPageTool` into
      `ExecutionEngine::AllowedSchemes`, which only Talk-To-Chrome sets to
      `kRequireHttpsOrHttpOrNtp`
      ([bac39efb0b61](https://chromium.googlesource.com/chromium/src/+/bac39efb0b6150b171a0b26d6f837cefd5d6996f)).
      `browser/ai_chat/content_agent_tool_provider.cc:87` and `:151` use the
      default. Unverified that Brave's agent ever targets the NTP.
- [ ] Check Brave branding on the Android graceful shutdown notification:
      `kTabAndroidGracefulShutdown` is on by default
      ([0303f5500d5a1](https://chromium.googlesource.com/chromium/src/+/0303f5500d5a1692bfd56a537cc526cfdc891f8a)),
      and starts `GracefulShutdownService` with the upstream icon and string
      when the last activity is destroyed during a slow tab shutdown. Check it
      against `BraveActivity.exitBrave()`.
- [ ] `patches/chrome-browser-net-proxy_config_monitor.cc.patch`: drop the
      `profile &&` in `if (profile && profile->IsTor())`, as upstream now
      `CHECK`s the profile
      ([83d732bd10f1b](https://chromium.googlesource.com/chromium/src/+/83d732bd10f1b11a957779d2cdf44a55d4efd559)).
      The same `[dcheck-to-check]` series touched eight other files Brave
      overrides (`system_network_context_manager.cc`,
      `profile_network_context_service.cc`, `stub_resolver_config_reader.cc`,
      `chrome_feature_list_creator.cc`, `chrome_metrics_service_client.cc`,
      `browsing_data_bridge.cc`, `autocomplete_controller_android.cc`,
      `execution_engine.cc`); I found no Tor or Brave profile that breaks the
      invariants, but violations now report until M161.
- [ ] Migrate Brave's tests off `CreateBrowserWithTestWindowForParams`
      (upstream's uses fell from 9 files to 5; not yet deprecated):
      `browser/profiles/brave_profile_manager_unittest.cc`,
      `browser/ui/brave_rewards/rewards_panel_coordinator_unittest.cc` and
      `browser/ui/commander/entity_match_unittest.cc`.
- [ ] Consider moving Python linting to ruff, as upstream did (17 PRESUBMIT
      files with `RunPylint` down to 2, a new `CheckRuff` in
      `tools/PRESUBMIT.py`;
      [a8ace9346d602](https://chromium.googlesource.com/chromium/src/+/a8ace9346d60226c9934262115fb3e9759163817)).
      Brave has `CheckPylint` (`PRESUBMIT.py:243`) and 255 `# pylint: disable`
      comments.

- [ ] `chromium_src/chrome/browser/ui/web_applications/app_browser_controller.cc:10`:
      upstream now calls `FormatUrlOrigin()` inside
      `AppBrowserController::GetLaunchFlashText()`
      ([8f46e634e52dd](https://chromium.googlesource.com/chromium/src/+/8f46e634e52dd5f4cc7ce855741ceedffbe5163a)),
      and the `FormatUrlOrigin_ChromiumImpl` rename makes that call skip
      `ReplaceChromeToBraveScheme`. Only reachable for a chrome-scheme opener of
      an app popup. Use a plaster on the function instead of the rename.
- [ ] Pass a `ui::ColorId` to `ui::ImageModel::FromVectorIcon` instead of
      resolving the color in `OnThemeChanged()`, as upstream keeps doing
      (crbug.com/394420459,
      [604563b50c707](https://chromium.googlesource.com/chromium/src/+/604563b50c7072b720f2f6f73064e57a4013ccba)):
      `browser/ui/views/infobars/web_discovery_infobar_content_view.cc:437`,
      `browser/ui/views/toolbar/wallet_button.cc:214`, `:218`.
- [ ] On the next run of `update-upstream-flake-filters.py`, check that the
      `ChromeBackForwardCacheBrowserWithEmbedPdfTest…/guestview_*` entries
      (`test/filters/generated/browser_tests-linux.filter:63`, `:66`, `:69`,
      `browser_tests-win.filter:78`, `:81`, `:84`) drop out: upstream deflaked
      them
      ([6f584ad691f2e](https://chromium.googlesource.com/chromium/src/+/6f584ad691f2eb64a509195f0898b8938a18495b)).
- [ ] `chromium_src/chrome/browser/resources/settings/appearance_page/appearance_page.html.ts.lit_mangler.ts:65`:
      the comment says the removed vertical tabs block includes the organizer
      panel toggle, which upstream moved to the top level
      ([6e83a7a6ce182](https://chromium.googlesource.com/chromium/src/+/6e83a7a6ce182f41057483265c66c26fc4d6c9e0)).
      Matters only if Brave enables `kOrganizerPanel`.

### 3-argument `BASE_FEATURE` with a redundant name

A presubmit error now rejects `BASE_FEATURE(kFoo, "Foo", ...)`, where the string
repeats the identifier
([c70964f1e8077](https://chromium.googlesource.com/chromium/src/+/c70964f1e8077),
[371696a6da55a](https://chromium.googlesource.com/chromium/src/+/371696a6da55a)).
Use `BASE_FEATURE(kFoo, ...)`. Brave's presubmit inlines upstream's, so the
check applies to lines Brave changes. No redundant uses remain.

- [x] `components/ai_chat/core/common/features.cc:228` `kAIChatDeepResearch`

## For privacy review

- `RTCDiagnosticLogging` shipped as stable. The browser side stays off through
  `kWebRtcEventLogCollectionAllowed`, but the JS surface is exposed.
- `kSmartSelectionServerSuggestions`, `kAutofillShowGmailOtpSuggestions`,
  `kZeroSuggestPrefetchOnPageLoadAndTabSwitch` and `kOnDeviceAiDefaultEnabled`
  (new, disabled).
- `GmailOtpOptInBubbleController`
  ([0c7ca2a62762d](https://chromium.googlesource.com/chromium/src/+/0c7ca2a62762d)):
  `TabFeatures` now creates it for every tab, ahead of the Gmail OTP launch.
  Nothing calls `SetUpAndShowBubble()` yet, so it stays dormant until a caller
  lands. `kPasskeyUnlockICloudRecovery` (new, disabled).
- `kCrossDeviceSigninFromDesktop` and `kDiceHeaderVersion2` (enabled): sign-in,
  already blocked by Brave's sign-in default.
- `kVmDetectionExperiment` (enabled, Windows UMA),
  `kAndroidSetGoogleAccountInHelp` (enabled), `kExtensionsPinnedByDefault`
  (enabled).
- New IPHs: `kIPHBookmarkBarVisibilityFeature`, `kIPHGlassFrameOptInFeature`,
  `kOmniboxFuseboxUserEd*Iph` (disabled).
- `kDeliveryOptimizationDownloader` (new, disabled, Windows): component
  downloads through Windows Delivery Optimization. It reaches Brave through
  `MakeCrxDownloaderFactory`
  (`browser/component_updater/brave_component_updater_configurator.cc:161`).
- `CrossOriginStorage` (new Blink runtime feature, not enabled;
  [05bb886d82871](https://chromium.googlesource.com/chromium/src/+/05bb886d82871)):
  a cross-origin file cache keyed by hash, which could bypass partitioning. The
  backend isn't wired yet; consider pinning it off.
- `kOneTimeTokenBackendNotification` (enabled on desktop;
  [80507db8839ee](https://chromium.googlesource.com/chromium/src/+/80507db8839ee)):
  advertised as a sharing feature in DeviceInfo. `kGmailOtpRetrievalService`
  stays pinned off.
- `kGlicReuseCookies`, `kFedCmAmbientBubble` and
  `kContextualSearchContextualCuesHandleEdu`/`Shopping` (new, disabled).
- `kExtensionProtocolHandlers` (enabled): extensions can register protocol
  handlers, and `protocol_handlers` is stable in manifest v3
  ([b47bdaeb9682a](https://chromium.googlesource.com/chromium/src/+/b47bdaeb9682aaf58c58e2763e0f962696e7517f)).
- `kUseFcmService` (new, disabled): a new FCM stack split out of `gcm_driver`
  ([090d50d04f0be](https://chromium.googlesource.com/chromium/src/+/090d50d04f0be838978fe9099d50ea91e2021c58)).
  It isn't covered by Brave's `brave.gcm.channel_status` pin.
- Windows DynamicPatching: Google-delivered patched child-module binaries
  through the component updater
  ([8d1650041cb13](https://chromium.googlesource.com/chromium/src/+/8d1650041cb1397ed180d6881e098bbb11adae09)).
  `kDynamicPatching` is pinned off, but if the component installer policy lands,
  its ID belongs in `kDisallowedComponents`
  (`chromium_src/components/component_updater/component_installer.cc:38`).
- `kSkillsServiceApi` (enabled, `chromeskills.pa.googleapis.com`), "Ask Google"
  and Contextual Tasks app menu entries
  ([af029b35f49ab](https://chromium.googlesource.com/chromium/src/+/af029b35f49ab9842292d351661b24528a6c8fa8)),
  and a New Badge for "Ask Google"
  ([359414146a8d4](https://chromium.googlesource.com/chromium/src/+/359414146a8d47333b5db1a78b372b7c059bd171)):
  all inert while Brave pins `kSkillsEnabled` and `kLensOverlay` off.
- Context Hub topics feedback export (behind `kContextHub`, disabled),
  `kDeviceAuthorizationStartupSilentFetch`, `kChangePasswordTool`,
  `kEnableAccountPreviewDataReducedTypes`, `kSyncWipeAppsAndAppSettingsData`,
  `kPersonalContextRequireSettingsToggleForEncryption` (new, disabled).
- `kEnableDesktopQrCodeDetection`: renderer-side image extraction for payment QR
  codes
  ([01d36342689ab](https://chromium.googlesource.com/chromium/src/+/01d36342689ab5ba13d8df68f8612f9f102eb0b9));
  not verified.
- `kCredentialManagementUnifiedUi` (enabled on desktop): new
  `navigator.credentials` password dialogs, with no Brave override.
- `CapabilityDelegationPopup` (new Blink runtime feature, experimental;
  [67250b2bb5931](https://chromium.googlesource.com/chromium/src/+/67250b2bb5931dc1869e0b172814ce6e3828d3b7)):
  a service worker can hand a window a one-second popup token after a
  notification click, past the popup blocker. Consider pinning it off if it
  ships.
- CryptAuth CMTG requests
  ([3d91d09bc3007](https://chromium.googlesource.com/chromium/src/+/3d91d09bc3007d209fc9d3b100edd65d4c181839)):
  need a primary-account OAuth token, so inert in Brave.
- `kGlicSsr` (X-Glic-Onboarding headers),
  `kTrustedVaultSharedRecoveryFactorsAndEncryption`,
  `kDeviceAuthorizationPasskeyDecryption`, `kDeviceTabVisibilitySettings` (new,
  disabled); `kAddSessionFallbackToPrimaryAccount` (enabled, Android Gaia
  ADDSESSION header, already blocked by Brave's sign-in default).
