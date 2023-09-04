/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/promoted_content_ads/promoted_content_ad_event_viewed.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/public/ads/promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/public/confirmation_type.h"

namespace brave_ads {

void PromotedContentAdEventViewed::FireEvent(const PromotedContentAdInfo& ad,
                                             ResultCallback callback) {
  LogAdEvent(ad, ConfirmationType::kViewed, std::move(callback));
}

}  // namespace brave_ads
