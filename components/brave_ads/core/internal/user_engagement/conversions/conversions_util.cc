/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_util.h"

#include "base/notreached.h"
#include "base/time/time.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_util_internal.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

namespace brave_ads {

bool IsAllowedToConvertAdEvent(const AdEventInfo& ad_event) {
  if (!CanConvertAdEvent(ad_event)) {
    return false;
  }

  switch (ad_event.type) {
    case mojom::AdType::kInlineContentAd:
    case mojom::AdType::kPromotedContentAd: {
      // Only if:
      // - The user has joined Brave News.
      return UserHasOptedInToBraveNewsAds();
    }

    case mojom::AdType::kNewTabPageAd: {
      // Only if:
      // - The user has opted into new tab page ads and has either joined Brave
      //   Rewards or new tab page ad events should always be triggered.
      return UserHasOptedInToNewTabPageAds() &&
             (UserHasJoinedBraveRewards() ||
              ShouldAlwaysTriggerNewTabPageAdEvents());
    }

    case mojom::AdType::kNotificationAd: {
      // Only if:
      // - The user has opted into notification ads. Users cannot opt into
      //   notification ads without joining Brave Rewards.
      return UserHasOptedInToNotificationAds();
    }

    case mojom::AdType::kSearchResultAd: {
      // Only if:
      // - The user has opted into search result ads and has either joined Brave
      //   Rewards or search result ad events should always be triggered.
      return UserHasOptedInToSearchResultAds() &&
             (UserHasJoinedBraveRewards() ||
              ShouldAlwaysTriggerSearchResultAdEvents());
    }

    case mojom::AdType::kUndefined: {
      break;
    }
  }

  NOTREACHED() << "Unexpected value for mojom::AdType: "
               << base::to_underlying(ad_event.type);
}

bool DidAdEventOccurWithinObservationWindow(
    const AdEventInfo& ad_event,
    const base::TimeDelta observation_window) {
  return ad_event.created_at >= base::Time::Now() - observation_window;
}

}  // namespace brave_ads
