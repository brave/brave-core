/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PREFS_PREF_PATH_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PREFS_PREF_PATH_UTIL_H_

#include <string>

namespace brave_ads {

[[nodiscard]] bool DoesMatchUserHasJoinedBraveRewardsPrefPath(
    const std::string& path);

[[nodiscard]] bool DoesMatchUserHasOptedInToBraveNewsAdsPrefPath(
    const std::string& path);

[[nodiscard]] bool DoesMatchUserHasOptedInToNewTabPageAdsPrefPath(
    const std::string& path);

[[nodiscard]] bool DoesMatchUserHasOptedInToNotificationAdsPrefPath(
    const std::string& path);

[[nodiscard]] bool DoesMatchUserHasOptedInToSearchResultAdsPrefPath(
    const std::string& path);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PREFS_PREF_PATH_UTIL_H_
