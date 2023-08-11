/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/eligible_ads_unittest_util.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_unittest_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"

namespace brave_ads {

void ResetEligibleAdsForTesting(const AdType& type) {
  ClientStateManager::GetInstance().ResetAllSeenAdsForType(type);

  ClientStateManager::GetInstance().ResetAllSeenAdvertisersForType(type);

  ResetAdEventsForTesting(
      base::BindOnce([](const bool success) { CHECK(success); }));
}

}  // namespace brave_ads
