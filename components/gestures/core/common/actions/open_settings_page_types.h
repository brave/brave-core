// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_OPEN_SETTINGS_PAGE_TYPES_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_OPEN_SETTINGS_PAGE_TYPES_H_

namespace gestures {

// Persisted directly in prefs, so never renumber or reorder, only append.
// Gaps below (e.g. 1-3, 7, 13) are skipped upstream values with no Brave route.
enum class OpenSettingsPageType {
  kBasic = 0,  // The default, today's unparameterized behavior.
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
  kAccessibility = 42,
  kSystem = 43,
  kSecurityKeys = 44,
  kSiteSettingsCamera = 45,
  kSiteSettingsLocation = 46,
  kSiteSettingsMicrophone = 47,
  kSiteSettingsNotifications = 48,
  kSiteSettingsJavascript = 49,
  kSiteSettingsImages = 50,
  kSiteSettingsPopups = 51,
  kSiteSettingsJavascriptOptimizer = 52,
  kMaxUpstreamValue = 999,

  // Specific to Brave, in `brave_routes.ts`; jumps to 1000 to leave room
  // above for future upstream subpages (see `kMaxUpstreamValue`).
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

  // `kExtensions` (1011) was removed, since `/extensions` redirects out to
  // `brave://extensions/`, never a settings page itself; left unused.
  kExtensionsV2 = 1012,
  kGetStarted = 1013,
  kOrigin = 1014,
  kLeoAi = 1015,
  kLeoAiCustomization = 1016,
  kLeoAiAddModel = 1017,

  // Like the block above, but nested under `kContentSettings`, matching how
  // the upstream `kSiteSettings*` entries nest under their own parent.
  kContentShields = 1018,

  kMaxValue = kContentShields,
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_OPEN_SETTINGS_PAGE_TYPES_H_
