/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Mirrors `OpenSettingsPage` in `open_settings_page_types.h`; persisted in
// prefs, so never renumber existing entries, only append new ones.
export enum OpenSettingsPage {
  BASIC = 0,

  // Below 100 mirrors an upstream route; values skipped here are routes
  // deliberately left out (dead, disabled by default, or unreachable).
  ALL_SITES = 4,
  APPEARANCE = 5,
  AUTOFILL = 6,
  AUTO_PICTURE_IN_PICTURE = 8,
  CLEAR_BROWSER_DATA = 9,
  CONTACT_INFO = 10,
  CONTENT_SETTINGS = 11,
  COOKIES = 12,
  DOWNLOADS = 14,
  FILE_SYSTEM_SETTINGS = 15,
  HANDLER_SETTINGS = 16,
  IMPORT_DATA = 17,
  LANGUAGES = 18,
  MANAGE_PROFILE = 20,
  ON_DEVICE_SITE_DATA = 21,
  PASSKEYS = 23,
  PASSWORD_MANAGER = 24,
  PAYMENTS = 25,
  PRIVACY_GUIDE = 27,
  PRIVACY = 29,
  RESET_PROFILE_SETTINGS = 30,
  RESET = 31,
  SAFE_BROWSING_ENHANCED_PROTECTION = 32,
  SAFETY_CHECK = 33,
  SEARCH_ENGINES = 34,
  SEARCH = 35,
  SECURITY = 36,
  SITE_DETAILS = 38,
  TRIGGERED_RESET_PROFILE_SETTINGS = 41,
  ACCESSIBILITY = 42,
  SYSTEM = 43,
  SECURITY_KEYS = 44,
  SITE_SETTINGS_CAMERA = 45,
  SITE_SETTINGS_LOCATION = 46,
  SITE_SETTINGS_MICROPHONE = 47,
  SITE_SETTINGS_NOTIFICATIONS = 48,
  SITE_SETTINGS_JAVASCRIPT = 49,
  SITE_SETTINGS_IMAGES = 50,
  SITE_SETTINGS_POPUPS = 51,
  SITE_SETTINGS_JAVASCRIPT_OPTIMIZER = 52,

  // Brave-only routes with no upstream equivalent; starts at 1000 to leave
  // room above for future upstream pages.
  SHORTCUTS = 1000,
  GESTURES = 1001,
  SHIELDS = 1002,
  SHIELDS_FILTERS = 1003,
  WEB3 = 1004,
  WALLET_NETWORKS = 1005,
  NEW_TAB = 1006,
  BRAVE_CONTENT = 1007,
  FONTS = 1008,
  BRAVE_SYNC = 1009,
  BRAVE_SYNC_SETUP = 1010,
  // `EXTENSIONS` (1011) was deliberately removed: `/extensions` redirects
  // out to `brave://extensions/`, an entirely different WebUI, so it was
  // never actually a settings page. 1011 is intentionally left unused
  // rather than reassigned.
  EXTENSIONS_V2 = 1012,
  GET_STARTED = 1013,
  ORIGIN = 1014,
  LEO_AI = 1015,
  LEO_AI_CUSTOMIZATION = 1016,
  LEO_AI_ADD_MODEL = 1017,
  // Specific to Brave, like the block above, but for a `content/*` subpage
  // (Shields' per-site permission status), so its gesture_path is nested under
  // `CONTENT_SETTINGS` the same way the upstream `SITE_SETTINGS_*` entries
  // above are nested under their own parent.
  CONTENT_SHIELDS = 1018,
}

// Shown directly in the dropdown instead of a translated name; kept in
// sync with `PageToWebUIPath` in `open_settings_page_web_ui_path_util.cc`.
export const OPEN_SETTINGS_PAGE_PATHS: Record<OpenSettingsPage, string> = {
  [OpenSettingsPage.BASIC]: '/',
  [OpenSettingsPage.ALL_SITES]: '/content/all',
  [OpenSettingsPage.APPEARANCE]: '/appearance',
  [OpenSettingsPage.AUTOFILL]: '/autofill',
  [OpenSettingsPage.AUTO_PICTURE_IN_PICTURE]:
    '/content/autoPictureInPicture',
  [OpenSettingsPage.CLEAR_BROWSER_DATA]: '/clearBrowserData',
  [OpenSettingsPage.CONTACT_INFO]: '/contactInfo',
  [OpenSettingsPage.CONTENT_SETTINGS]: '/content',
  [OpenSettingsPage.COOKIES]: '/cookies',
  [OpenSettingsPage.DOWNLOADS]: '/downloads',
  [OpenSettingsPage.FILE_SYSTEM_SETTINGS]: '/content/filesystem/siteDetails',
  [OpenSettingsPage.HANDLER_SETTINGS]: '/handlers',
  [OpenSettingsPage.IMPORT_DATA]: '/importData',
  [OpenSettingsPage.LANGUAGES]: '/languages',
  [OpenSettingsPage.MANAGE_PROFILE]: '/manageProfile',
  [OpenSettingsPage.ON_DEVICE_SITE_DATA]: '/content/siteData',
  [OpenSettingsPage.PASSKEYS]: '/passkeys',
  [OpenSettingsPage.PASSWORD_MANAGER]: '/passwords',
  [OpenSettingsPage.PAYMENTS]: '/payments',
  [OpenSettingsPage.PRIVACY_GUIDE]: '/privacy/guide',
  [OpenSettingsPage.PRIVACY]: '/privacy',
  [OpenSettingsPage.RESET_PROFILE_SETTINGS]: '/resetProfileSettings',
  [OpenSettingsPage.RESET]: '/reset',
  [OpenSettingsPage.SAFE_BROWSING_ENHANCED_PROTECTION]:
    '/security?q=enhanced',
  [OpenSettingsPage.SAFETY_CHECK]: '/safetyCheck',
  [OpenSettingsPage.SEARCH_ENGINES]: '/searchEngines',
  [OpenSettingsPage.SEARCH]: '/search',
  [OpenSettingsPage.SECURITY]: '/security',
  [OpenSettingsPage.SITE_DETAILS]: '/content/siteDetails',
  [OpenSettingsPage.TRIGGERED_RESET_PROFILE_SETTINGS]:
    '/triggeredResetProfileSettings',
  [OpenSettingsPage.ACCESSIBILITY]: '/accessibility',
  [OpenSettingsPage.SYSTEM]: '/system',
  [OpenSettingsPage.SECURITY_KEYS]: '/securityKeys',
  [OpenSettingsPage.SITE_SETTINGS_CAMERA]: '/content/camera',
  [OpenSettingsPage.SITE_SETTINGS_LOCATION]: '/content/location',
  [OpenSettingsPage.SITE_SETTINGS_MICROPHONE]: '/content/microphone',
  [OpenSettingsPage.SITE_SETTINGS_NOTIFICATIONS]: '/content/notifications',
  [OpenSettingsPage.SITE_SETTINGS_JAVASCRIPT]: '/content/javascript',
  [OpenSettingsPage.SITE_SETTINGS_IMAGES]: '/content/images',
  [OpenSettingsPage.SITE_SETTINGS_POPUPS]: '/content/popups',
  [OpenSettingsPage.SITE_SETTINGS_JAVASCRIPT_OPTIMIZER]: '/content/v8',
  [OpenSettingsPage.SHORTCUTS]: '/system/shortcuts',
  [OpenSettingsPage.GESTURES]: '/system/gestures',
  [OpenSettingsPage.SHIELDS]: '/shields',
  [OpenSettingsPage.SHIELDS_FILTERS]: '/shields/filters',
  [OpenSettingsPage.WEB3]: '/web3',
  [OpenSettingsPage.WALLET_NETWORKS]: '/wallet/networks',
  [OpenSettingsPage.NEW_TAB]: '/newTab',
  [OpenSettingsPage.BRAVE_CONTENT]: '/braveContent',
  [OpenSettingsPage.FONTS]: '/fonts',
  [OpenSettingsPage.BRAVE_SYNC]: '/braveSync',
  [OpenSettingsPage.BRAVE_SYNC_SETUP]: '/braveSync/setup',
  [OpenSettingsPage.EXTENSIONS_V2]: '/extensions/v2',
  [OpenSettingsPage.GET_STARTED]: '/getStarted',
  [OpenSettingsPage.ORIGIN]: '/origin',
  [OpenSettingsPage.LEO_AI]: '/leo-ai',
  [OpenSettingsPage.LEO_AI_CUSTOMIZATION]: '/leo-ai/customization',
  [OpenSettingsPage.LEO_AI_ADD_MODEL]: '/leo-ai/add-model',
  [OpenSettingsPage.CONTENT_SHIELDS]: '/content/braveShields',
}

// Order of the flat page dropdown; alphabetical by gesture_path, except `BASIC`,
// which stays first as the default.
export const OPEN_SETTINGS_PAGE_ORDER: OpenSettingsPage[] = [
  OpenSettingsPage.BASIC,
  ...Object.values(OpenSettingsPage)
    .filter(
      (page): page is OpenSettingsPage =>
        typeof page === 'number' && page !== OpenSettingsPage.BASIC)
    .sort((a, b) =>
      OPEN_SETTINGS_PAGE_PATHS[a].localeCompare(OPEN_SETTINGS_PAGE_PATHS[b])),
]
