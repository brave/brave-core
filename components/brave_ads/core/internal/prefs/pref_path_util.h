/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PREFS_PREF_PATH_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PREFS_PREF_PATH_UTIL_H_

#include <string_view>

namespace brave_ads {

bool DoesMatchUserHasJoinedBraveRewardsPrefPath(std::string_view path);

bool DoesMatchUserHasOptedInToNewTabPageAdsPrefPath(std::string_view path);

bool DoesMatchUserHasOptedInToNotificationAdsPrefPath(std::string_view path);

bool DoesMatchUserHasOptedInToSearchResultAdsPrefPath(std::string_view path);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PREFS_PREF_PATH_UTIL_H_
