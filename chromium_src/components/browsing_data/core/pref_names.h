/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_BROWSING_DATA_CORE_PREF_NAMES_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_BROWSING_DATA_CORE_PREF_NAMES_H_

#include "src/components/browsing_data/core/pref_names.h"  // IWYU pragma: export

namespace browsing_data {

namespace prefs {

inline constexpr char kDeleteBrowsingHistoryOnExit[] =
    "browser.clear_data.browsing_history_on_exit";
inline constexpr char kDeleteDownloadHistoryOnExit[] =
    "browser.clear_data.download_history_on_exit";
inline constexpr char kDeleteCacheOnExit[] = "browser.clear_data.cache_on_exit";
inline constexpr char kDeleteCookiesOnExit[] =
    "browser.clear_data.cookies_on_exit";
inline constexpr char kDeletePasswordsOnExit[] =
    "browser.clear_data.passwords_on_exit";
inline constexpr char kDeleteFormDataOnExit[] =
    "browser.clear_data.form_data_on_exit";
inline constexpr char kDeleteHostedAppsDataOnExit[] =
    "browser.clear_data.hosted_apps_data_on_exit";
inline constexpr char kDeleteSiteSettingsOnExit[] =
    "browser.clear_data.site_settings_on_exit";
inline constexpr char kDeleteBraveLeoHistory[] = "browser.clear_data.brave_leo";
inline constexpr char kDeleteBraveLeoHistoryOnExit[] =
    "browser.clear_data.brave_leo_on_exit";

}  // namespace prefs

}  // namespace browsing_data

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_BROWSING_DATA_CORE_PREF_NAMES_H_
