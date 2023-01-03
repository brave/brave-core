/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/eligible_ads_unittest_util.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/internal/ads/ad_events/ad_events_database_table_unittest_util.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"

namespace ads {

void ResetEligibleAds(const AdType& type) {
  ClientStateManager::GetInstance()->ResetAllSeenAdsForType(type);

  ClientStateManager::GetInstance()->ResetAllSeenAdvertisersForType(type);

  database::table::ad_events::Reset(
      base::BindOnce([](const bool success) { CHECK(success); }));
}

}  // namespace ads
