// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_OPEN_SETTINGS_PAGE_TYPES_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_OPEN_SETTINGS_PAGE_TYPES_H_

namespace gestures {

// A curated selection of settings pages that can be opened, not every
// possible settings page. Persisted directly in prefs as `IntParamInfo`
// (see `kValueParamKey` in `int_param_util.h`), so, like
// `GestureActionInfo::command`, never renumber or reorder existing
// entries; only append new ones. Mapped to an
// actual settings WebUI relative path by `PageToWebUIPath` in
// `gesture_action_dispatcher.cc`. Kept in sync with `OpenSettingsPage` in
// `open_settings_page.ts`.
//
// Values below 100 mirror an upstream `chrome::k*SubPage` constant; values
// from 100 on are routes specific to Brave with no upstream equivalent (see
// `brave_routes.ts`). This split reserves room in values 1 through 99 for
// future upstream pages Brave wants to expose, so they don't have to be
// appended after (and numbered higher than) the block specific to Brave.
enum class OpenSettingsPage {
  kBasic = 0,  // The default, today's unparameterized behavior.

  // Mirrors an upstream `chrome::k*SubPage` constant. `kAccount`,
  // `kAddresses` (a dead alias of `kContactInfo`), `kAdPrivacy`,
  // `kAutofillAi` (the only route at that path is a Google Gemini-only
  // feature, disabled by default), `kDefaultBrowser`, `kLanguageDetails`,
  // `kOnStartup`, `kPeople`, `kPrivacySandboxMeasurement`, `kShopping`
  // (disabled by default, same underlying feature as `kAutofillAi`),
  // `kSignOut`, and `kSyncSetup` were deliberately left out. Their routes
  // are never constructed in a real Brave build, either because Brave's
  // own page visibility overrides (`br/page_visibility.ts`) prevent the
  // route object from ever being created, because the controlling feature
  // is disabled by default with no Brave override enabling it, or because
  // the upstream constant has no matching route at all.
  kAllSites = 4,
  kAppearance = 5,
  kAutofill = 6,
  kAutoPictureInPicture = 8,
  kClearBrowserData = 9,
  kContactInfo = 10,
  kContentSettings = 11,
  kCookies = 12,
  kDownloads = 14,
  kFileSystemSettings = 15,
  kHandlerSettings = 16,
  kImportData = 17,
  kLanguages = 18,
  kManageProfile = 20,
  kOnDeviceSiteData = 21,
  kPasskeys = 23,
  kPasswordManager = 24,
  kPayments = 25,
  kPrivacyGuide = 27,
  kPrivacy = 29,
  kResetProfileSettings = 30,
  kReset = 31,
  kSafeBrowsingEnhancedProtection = 32,
  kSafetyCheck = 33,
  kSearchEngines = 34,
  kSearch = 35,
  kSecurity = 36,
  kSiteDetails = 38,
  kTriggeredResetProfileSettings = 41,
  kMaxUpstreamValue = 999,

  // Specific to Brave, not backed by a `chrome::k*SubPage` constant, since
  // these routes only exist in Brave's own `brave_routes.ts`. Jumps to 1000
  // to leave room above for future upstream subpages (see
  // `kMaxUpstreamValue`).
  kShortcuts = 1000,
  kGestures = 1001,
  kShields = 1002,
  kShieldsFilters = 1003,
  kWeb3 = 1004,
  kWalletNetworks = 1005,
  kNewTab = 1006,
  kBraveContent = 1007,
  kFonts = 1008,
  kBraveSync = 1009,
  kBraveSyncSetup = 1010,
  kExtensions = 1011,
  kExtensionsV2 = 1012,
  kGetStarted = 1013,
  kOrigin = 1014,
  kLeoAi = 1015,
  kLeoAiCustomization = 1016,
  kLeoAiAddModel = 1017,
  kMaxValue = kLeoAiAddModel,
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_OPEN_SETTINGS_PAGE_TYPES_H_
