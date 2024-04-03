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
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_feature.h"

namespace brave_ads {

bool CanConvertAdEvent(const AdEventInfo& ad_event) {
  // Only convert view-through and click-through ad events.
  if (ad_event.confirmation_type != ConfirmationType::kViewedImpression &&
      ad_event.confirmation_type != ConfirmationType::kClicked) {
    return false;
  }

  switch (ad_event.type) {
    case AdType::kInlineContentAd:
    case AdType::kPromotedContentAd: {
      return UserHasOptedInToBraveNewsAds();
    }

    case AdType::kNewTabPageAd: {
      return UserHasOptedInToNewTabPageAds();
    }

    case AdType::kNotificationAd: {
      return UserHasOptedInToNotificationAds();
    }

    case AdType::kSearchResultAd: {
      // Always.
      return true;
    }

    case AdType::kUndefined: {
      break;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for AdType: "
                        << base::to_underlying(ad_event.type);
}

bool HasObservationWindowForAdEventExpired(
    const base::TimeDelta observation_window,
    const AdEventInfo& ad_event) {
  return ad_event.created_at < base::Time::Now() - observation_window;
}

}  // namespace brave_ads
