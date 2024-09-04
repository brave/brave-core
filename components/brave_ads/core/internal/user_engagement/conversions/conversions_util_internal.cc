/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_util_internal.h"

#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

bool CanConvertAdEvent(const AdEventInfo& ad_event) {
  if (UserHasJoinedBraveRewards()) {
    // For Rewards users, allow both view-through and click-through conversions.
    return ad_event.confirmation_type ==
               mojom::ConfirmationType::kViewedImpression ||
           ad_event.confirmation_type == mojom::ConfirmationType::kClicked;
  }

  if (ad_event.type == mojom::AdType::kInlineContentAd ||
      ad_event.type == mojom::AdType::kPromotedContentAd) {
    // For non-Rewards users, allow view-through and click-through conversions
    // for inline content and promoted content ads.
    return ad_event.confirmation_type ==
               mojom::ConfirmationType::kViewedImpression ||
           ad_event.confirmation_type == mojom::ConfirmationType::kClicked;
  }

  // Otherwise, only allow click-through conversions.
  return ad_event.confirmation_type == mojom::ConfirmationType::kClicked;
}

}  // namespace brave_ads
