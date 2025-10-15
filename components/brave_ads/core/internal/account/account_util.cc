/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/account_util.h"

#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

bool IsAllowedToDeposit(const std::string& creative_instance_id,
                        mojom::AdType mojom_ad_type,
                        mojom::ConfirmationType mojom_confirmation_type) {
  if (mojom_ad_type == mojom::AdType::kNewTabPageAd &&
      !ShouldReportMetric(creative_instance_id)) {
    // Do not allow deposits for new tab page ads if we should not report
    // metrics for the given creative instance.
    return false;
  }

  if (UserHasJoinedBraveRewards()) {
    // Always allow deposits for Rewards users.
    return true;
  }

  // Evaluate deposit eligibility for non-Rewards users based on ad type and
  // other conditions.
  switch (mojom_ad_type) {
    case mojom::AdType::kInlineContentAd:
    case mojom::AdType::kPromotedContentAd: {
      // Only allow deposits for non-Rewards users who have joined Brave News.
      return UserHasOptedInToBraveNewsAds();
    }

    case mojom::AdType::kNewTabPageAd: {
      // Only allow deposits for non-Rewards users who have opted into new tab
      // page ads.
      return UserHasOptedInToNewTabPageAds();
    }

    case mojom::AdType::kNotificationAd: {
      // Never allow deposits because users cannot opt into notification ads
      // without joining Brave Rewards.
      return false;
    }

    case mojom::AdType::kSearchResultAd: {
      // Only allow conversion deposits for non-Rewards users.
      return mojom_confirmation_type == mojom::ConfirmationType::kConversion;
    }

    case mojom::AdType::kUndefined: {
      break;
    }
  }

  NOTREACHED() << "Unexpected value for mojom::AdType: "
               << base::to_underlying(mojom_ad_type);
}

}  // namespace brave_ads
