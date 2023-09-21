/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_BROWSING_DATA_CORE_PREF_NAMES_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_BROWSING_DATA_CORE_PREF_NAMES_H_

#include "src/components/browsing_data/core/pref_names.h"  // IWYU pragma: export

namespace browsing_data {

namespace prefs {

extern const char kDeleteBrowsingHistoryOnExit[];
extern const char kDeleteDownloadHistoryOnExit[];
extern const char kDeleteCacheOnExit[];
extern const char kDeleteCookiesOnExit[];
extern const char kDeletePasswordsOnExit[];
extern const char kDeleteFormDataOnExit[];
extern const char kDeleteHostedAppsDataOnExit[];
extern const char kDeleteSiteSettingsOnExit[];
constexpr char kDeleteBraveLeoHistory[] = "browser.clear_data.brave_leo";
constexpr char kDeleteBraveLeoHistoryOnExit[] =
    "browser.clear_data.brave_leo_on_exit";
}  // namespace prefs

}  // namespace browsing_data

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_BROWSING_DATA_CORE_PREF_NAMES_H_
