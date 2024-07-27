/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/account_util.h"

#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

namespace brave_ads {

bool IsAllowedToDeposit(AdType ad_type, ConfirmationType confirmation_type) {
  if (UserHasJoinedBraveRewards()) {
    // Always allow deposits for Rewards users.
    return true;
  }

  switch (ad_type) {
    case AdType::kInlineContentAd:
    case AdType::kPromotedContentAd: {
      // Only allow deposits for non-Rewards users who have joined Brave News.
      return UserHasOptedInToBraveNewsAds();
    }

    case AdType::kNewTabPageAd: {
      // Only allow deposits for non-Rewards users if
      // brave://flags/#brave-ads-should-always-trigger-new-tab-page-ad-events
      // is enabled.
      return ShouldAlwaysTriggerNewTabPageAdEvents();
    }

    case AdType::kNotificationAd: {
      // Never allow deposits because users cannot opt into notification ads
      // without joining Brave Rewards.
      return false;
    }

    case AdType::kSearchResultAd: {
      // Only allow conversion deposits for non-Rewards users if
      // brave://flags/#brave-ads-should-always-trigger-search-result-ad-events
      // is enabled.
      return ShouldAlwaysTriggerSearchResultAdEvents() &&
             confirmation_type == ConfirmationType::kConversion;
    }

    case AdType::kUndefined: {
      break;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for AdType: "
                        << base::to_underlying(ad_type);
}

}  // namespace brave_ads
