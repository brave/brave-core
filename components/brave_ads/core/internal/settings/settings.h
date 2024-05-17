/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SETTINGS_SETTINGS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SETTINGS_SETTINGS_H_

namespace brave_ads {

bool UserHasJoinedBraveRewards();

bool UserHasOptedInToBraveNewsAds();

bool UserHasOptedInToNewTabPageAds();

bool UserHasOptedInToNotificationAds();
int GetMaximumNotificationAdsPerHour();

bool UserHasOptedInToSearchResultAds();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SETTINGS_SETTINGS_H_
