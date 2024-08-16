/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/prefs/pref_path_util.h"

#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"

namespace brave_ads {

bool DoesMatchUserHasJoinedBraveRewardsPrefPath(const std::string& path) {
  return path == brave_rewards::prefs::kEnabled;
}

bool DoesMatchUserHasOptedInToBraveNewsAdsPrefPath(const std::string& path) {
  return path == brave_news::prefs::kBraveNewsOptedIn ||
         path == brave_news::prefs::kNewTabPageShowToday;
}

bool DoesMatchUserHasOptedInToNewTabPageAdsPrefPath(const std::string& path) {
  return path == ntp_background_images::prefs::kNewTabPageShowBackgroundImage ||
         path == ntp_background_images::prefs::
                     kNewTabPageShowSponsoredImagesBackgroundImage;
}

bool DoesMatchUserHasOptedInToNotificationAdsPrefPath(const std::string& path) {
  return path == prefs::kOptedInToNotificationAds;
}

bool DoesMatchUserHasOptedInToSearchResultAdsPrefPath(const std::string& path) {
  return path == prefs::kOptedInToSearchResultAds;
}

}  // namespace brave_ads
