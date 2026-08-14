// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SHARED_PREFS_PREF_NAMES_H_
#define BRAVE_IOS_BROWSER_SHARED_PREFS_PREF_NAMES_H_

namespace brave_shields {

// The level of HTTPS upgrades applied to main frame navigations. Values are
// persisted, do not renumber.
enum class HttpsUpgradeLevel {
  // No navigations are upgraded
  kDisabled = 0,
  // Navigations are upgraded, falling back to HTTP automatically when the
  // upgraded navigation fails
  kStandard = 1,
  // Navigations are upgraded, showing an interstitial the user must accept
  // before falling back to HTTP
  kStrict = 2,
};

}  // namespace brave_shields

namespace prefs {

// The `brave_shields::HttpsUpgradeLevel` applied to navigations
inline constexpr char kHttpsUpgradeLevel[] = "brave.https_upgrade_level";

// Whether or not videos continue to play when the app is backgrounded
inline constexpr char kMediaBackgroundingEnabled[] =
    "brave.media_backgrounding_enabled";

// Whether or not to block all cookies and access to local storage
inline constexpr char kBlockAllCookiesEnabled[] =
    "brave.block_all_cookies_enabled";

}  // namespace prefs

#endif  // BRAVE_IOS_BROWSER_SHARED_PREFS_PREF_NAMES_H_
