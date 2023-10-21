/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/promoted_content_ads/promoted_content_ad_event_clicked.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/units/promoted_content_ad/promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

namespace brave_ads {

void PromotedContentAdEventClicked::FireEvent(const PromotedContentAdInfo& ad,
                                              ResultCallback callback) {
  RecordAdEvent(ad, ConfirmationType::kClicked, std::move(callback));
}

}  // namespace brave_ads
